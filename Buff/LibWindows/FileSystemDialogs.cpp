#include <ShlObj.h>
// shobj needs to be first, because we undefine CONST macro later
#include "LibWindows/FileSystemDialogs.h"
#include "LibWindows/Platform.h"
#include <commdlg.h>
#include <Shlwapi.h>

// This fixes "Missing ordinal 344" error
// https://stackoverflow.com/questions/43426776/the-ordinal-344-could-not-be-located-in-the-dynamic-link-library
#pragma comment(                                                                                             \
    linker,                                                                                                  \
    "\"/manifestdependency:type='Win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

BUFF_NAMESPACE_BEGIN

BUFF_DISABLE_CLANG_WARNING_BEGIN("-Wlanguage-extension-token")

/// Written from some online example, possibly chatgpt
class DialogEventHandler final
    : public IFileDialogEvents
    , public IFileDialogControlEvents {

    long mRef = 1;

public:
    // IUnknown methods
    virtual IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        static const QITAB QIT[] = {
            QITABENT(DialogEventHandler, IFileDialogEvents),
            QITABENT(DialogEventHandler, IFileDialogControlEvents),
            {},
        };
        return QISearch(this, QIT, riid, ppv);
    }

    virtual IFACEMETHODIMP_(ULONG) AddRef() override {
        return InterlockedIncrement(&mRef);
    }

    virtual IFACEMETHODIMP_(ULONG) Release() override {
        const long ref = InterlockedDecrement(&mRef);
        if (!ref) {
            delete this;
        }
        return ref;
    }

    // =======================================================================================================
    // IFileDialogEvents methods
    // =======================================================================================================

    virtual IFACEMETHODIMP OnFileOk(IFileDialog*) override {
        return S_OK;
    }
    virtual IFACEMETHODIMP OnFolderChange(IFileDialog*) override {
        return S_OK;
    }
    virtual IFACEMETHODIMP OnFolderChanging(IFileDialog*, IShellItem*) override {
        return S_OK;
    }
    virtual IFACEMETHODIMP OnSelectionChange(IFileDialog*) override {
        return S_OK;
    }
    virtual IFACEMETHODIMP OnShareViolation(IFileDialog*,
                                            IShellItem*,
                                            FDE_SHAREVIOLATION_RESPONSE*) override {
        return S_OK;
    }
    virtual IFACEMETHODIMP OnTypeChange(IFileDialog* BUFF_UNUSED(pfd)) override {
        return S_OK;
    }
    virtual IFACEMETHODIMP OnOverwrite(IFileDialog*, IShellItem*, FDE_OVERWRITE_RESPONSE*) override {
        return S_OK;
    }

    // =======================================================================================================
    // IFileDialogControlEvents methods
    // =======================================================================================================

    virtual IFACEMETHODIMP OnItemSelected(IFileDialogCustomize*, DWORD, DWORD) override {
        return S_OK;
    }
    virtual IFACEMETHODIMP OnButtonClicked(IFileDialogCustomize*, DWORD) override {
        return S_OK;
    }
    virtual IFACEMETHODIMP OnCheckButtonToggled(IFileDialogCustomize*, DWORD, BOOL) override {
        return S_OK;
    }
    virtual IFACEMETHODIMP OnControlActivating(IFileDialogCustomize*, DWORD) override {
        return S_OK;
    }
};

struct AllDialogParams {
    FilesystemDialogOptions common;
    enum class Type {
        OPEN_FILE,
        SAVE_FILE,
        FOLDER,
    };
    using enum Type;
    Type type;

    SelectFolderDialogOptions folder;

    SelectFileDialogOptions file;
};

struct Result {
    Optional<String> path;
    int              selectedFileType = -1;
};

static Result dialogImpl(const AllDialogParams& params) {

    // =======================================================================================================
    // Basic setup
    // =======================================================================================================

    // CoCreate the File Open Dialog object.
    IFileDialog* fileDialog = nullptr;
    BUFF_CHECKED_CALL_HRESULT(CoCreateInstance(
        params.type == AllDialogParams::SAVE_FILE ? CLSID_FileSaveDialog : CLSID_FileOpenDialog,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&fileDialog)));
    const Finally finally1([&] { fileDialog->Release(); });

    // Create an event handling object, and hook it up to the dialog.
    IFileDialogEvents*  dialogEvents        = nullptr;
    DialogEventHandler* dialogEventsHandler = new (std::nothrow) DialogEventHandler();
    BUFF_ASSERT(dialogEventsHandler);
    BUFF_CHECKED_CALL_HRESULT(dialogEventsHandler->QueryInterface(IID_PPV_ARGS(&dialogEvents)));
    const Finally finally2([&] { dialogEvents->Release(); });
    dialogEventsHandler->Release();

    // Hook up the event handler.
    DWORD cookie;
    BUFF_CHECKED_CALL_HRESULT(fileDialog->Advise(dialogEvents, &cookie));
    const Finally finally3([&] { fileDialog->Unadvise(cookie); });

    // =======================================================================================================
    // Setting options
    // =======================================================================================================

    BUFF_CHECKED_CALL_HRESULT(fileDialog->SetFileNameLabel(
        params.type == AllDialogParams::FOLDER ? L"Folder name:" : L"File name:"));

    const std::wstring title = params.common.title.asWString();
    BUFF_CHECKED_CALL_HRESULT(fileDialog->SetTitle(title.c_str()));

    // Set the options on the dialog.
    DWORD flags;
    // Before setting, always get the options first in order not to override existing options.
    BUFF_CHECKED_CALL_HRESULT(fileDialog->GetOptions(&flags));
    flags |= FOS_FORCEFILESYSTEM; // Only filesystem items
    switch (params.type) {
    case AllDialogParams::FOLDER:
        flags |= FOS_PICKFOLDERS;
        if (!params.folder.mustExist) {
            flags &= FOS_PATHMUSTEXIST;
        }
        break;
    case AllDialogParams::OPEN_FILE:
        break;
    case AllDialogParams::SAVE_FILE:
        flags &= FOS_PATHMUSTEXIST;
        break;
    }

    // Future options: FOS_ALLOWMULTISELECT
    BUFF_CHECKED_CALL_HRESULT(fileDialog->SetOptions(flags));

    Array<std::wstring>      fileTypeStrings;
    Array<COMDLG_FILTERSPEC> fileTypes;
    if (params.type != AllDialogParams::FOLDER) {
        fileTypeStrings.reserve(params.file.fileTypes.size() * 2);
        fileTypes.reserve(params.file.fileTypes.size());
        for (const auto& type : params.file.fileTypes) {
            fileTypes.pushBack();
            COMDLG_FILTERSPEC& out         = fileTypes.back();
            const String extensionsPattern = listToStr(type.extensions, ", ", [](const String& extension) {
                BUFF_ASSERT(!extension.contains("*") && !extension.contains(";"));
                return "*." + extension;
            });
            fileTypeStrings.pushBack((type.description + " (" + extensionsPattern + ")").asWString());
            out.pszName = fileTypeStrings.back().c_str();
            fileTypeStrings.pushBack(extensionsPattern.getWithReplaceAll(", ", ";").asWString());
            out.pszSpec = fileTypeStrings.back().c_str();
        }
        // Set the file types to display only. Notice that this is a 1-based array.
        BUFF_CHECKED_CALL_HRESULT(fileDialog->SetFileTypes(int(fileTypes.size()), fileTypes.data()));
        if (params.file.selectedType) {
            BUFF_CHECKED_CALL_HRESULT(fileDialog->SetFileTypeIndex(*params.file.selectedType + 1));
        }
    }

    if (params.common.initialFolder) {
        IShellItem* defaultFolder = nullptr;
        BUFF_CHECKED_CALL_HRESULT(
            SHCreateItemFromParsingName(params.common.initialFolder->getNative().asWString().c_str(),
                                        nullptr,
                                        IID_PPV_ARGS(&defaultFolder)));
        BUFF_CHECKED_CALL_HRESULT(fileDialog->SetDefaultFolder(defaultFolder));
        BUFF_CHECKED_CALL_HRESULT(fileDialog->SetFolder(defaultFolder));
        defaultFolder->Release();
    }

    std::wstring preTyped;
    if (params.common.preTypedName) {
        preTyped = params.common.preTypedName->asWString();
        BUFF_CHECKED_CALL_HRESULT(fileDialog->SetFileName(preTyped.c_str()));
    }

    std::wstring okButtonLabel;
    if (params.common.okButtonText) {
        okButtonLabel = params.common.okButtonText->asWString();
        BUFF_CHECKED_CALL_HRESULT(fileDialog->SetOkButtonLabel(okButtonLabel.c_str()));
    }

    // Set the default extension to be ".doc" file.
    // BUFF_CHECKED_CALL_HRESULT(fileDialog->SetDefaultExtension(L"doc;docx"));

    // =======================================================================================================
    // Execution
    // =======================================================================================================

    // Show the dialog
    const HRESULT result = fileDialog->Show(HWND(params.common.parentWindow.handle));
    if (SUCCEEDED(result)) {
        IShellItem* path;
        BUFF_CHECKED_CALL_HRESULT(fileDialog->GetResult(&path));
        const Finally finally4([&] { path->Release(); });

        uint selectedType;
        BUFF_CHECKED_CALL_HRESULT(fileDialog->GetFileTypeIndex(&selectedType));

        // We are just going to print out the name of the file for sample sake.
        PWSTR resultValue = nullptr;
        BUFF_CHECKED_CALL_HRESULT(path->GetDisplayName(SIGDN_FILESYSPATH, &resultValue));
        const Finally finally5([&] { CoTaskMemFree(resultValue); });
        return {
            .path             = String(resultValue),
            .selectedFileType = int(selectedType - 1),
        };
    } else {
        BUFF_ASSERT(HRESULT_CODE(result) == ERROR_CANCELLED);
        return {};
    }
}

Optional<FilePath> displaySaveFileDialog(const FilesystemDialogOptions& common,
                                         const SelectFileDialogOptions& specific) {
    const AllDialogParams params {
        .common = common,
        .type   = AllDialogParams::SAVE_FILE,
        .file   = specific,
    };
    const Result res = dialogImpl(params);
    if (res.path) {
        FilePath adjusted(*res.path);
        if (!adjusted.getExtension()) {
            const int filterIndex = res.selectedFileType;
            adjusted = FilePath(adjusted.getGeneric() + "." + specific.fileTypes[filterIndex].extensions[0]);
        }
        return adjusted;

    } else {
        return {};
    }
}

Optional<DirectoryPath> displaySelectFolderDialog(const FilesystemDialogOptions&   common,
                                                  const SelectFolderDialogOptions& specific) {
    const AllDialogParams params {
        .common = common,
        .type   = AllDialogParams::FOLDER,
        .folder = specific,
    };

    const Result res = dialogImpl(params);
    if (res.path) {
        return DirectoryPath(*res.path);
    } else {
        return NULL_OPTIONAL;
    }
}

BUFF_DISABLE_CLANG_WARNING_END()

BUFF_NAMESPACE_END
