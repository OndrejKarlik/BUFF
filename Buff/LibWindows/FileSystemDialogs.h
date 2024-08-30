#pragma once
#include "Lib/Path.h"
#include "Lib/Platform.h"

BUFF_NAMESPACE_BEGIN

struct FileType {

    /// Do not replicate the allowed extensions here - it gets generated automatically
    String description;

    /// Just list of extensions, do not include *. part here
    Array<String> extensions;
};

struct FilesystemDialogOptions {
    String                  title;
    NativeWindowHandle      parentWindow;
    Optional<DirectoryPath> initialFolder;
    // Folder or directory that is pre-typed in the input box, inside the current folder. Does not need to
    // exist.
    Optional<String> preTypedName;

    /// If present, overrides default confirm button text (e.g. "Open", "Save")
    Optional<String> okButtonText;
};

struct SelectFileDialogOptions {
    ArrayView<const FileType> fileTypes;
    Optional<int>             selectedType;
};

// TODO: pre-select filename
// TODO: allow all files
Optional<FilePath> displaySaveFileDialog(const FilesystemDialogOptions& common,
                                         const SelectFileDialogOptions& specific);

struct SelectFolderDialogOptions {
    bool mustExist = false;
};

Optional<DirectoryPath> displaySelectFolderDialog(const FilesystemDialogOptions&   common,
                                                  const SelectFolderDialogOptions& specific = {});

BUFF_NAMESPACE_END
