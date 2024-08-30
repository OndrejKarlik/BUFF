#pragma once
#include "LibWindows/DirectoryModificationWatcher.h"
#include "Lib/AutoPtr.h"
#include "Lib/Exception.h"
#include "Lib/Filesystem.h"
#include "Lib/Path.h"
#include <functional>
#include <shellapi.h>
#ifdef __EMSCRIPTEN__
#    include "LibEmscripten/LibEmscripten.h"
#else
#    include "LibWindows/AssertHandler.h"
#endif

BUFF_NAMESPACE_BEGIN

/// Returned by call to HotswapInterfaceFn getHotswappableInterface
using HotswapInterfaceFn = Polymorphic* (*)();

class HotswapHost {
    Function<void()>                      mDllChangeCallback;
    AutoPtr<DirectoryModificationWatcher> mDirWatcher;
    DirectoryPath                         mTemporaryDir;

    FilePath  mDllToLoadAndWatch;
    FilePath  mTemporaryDll;
    HINSTANCE mLoadedDll = nullptr;

    FilePath mOriginalPdb;
    FilePath mTemporaryPdb;

public:
    HotswapHost(const FilePath& dllToWatch, Function<void()> dllChangeCallback)
        : mDllChangeCallback(std::move(dllChangeCallback))
        , mDllToLoadAndWatch(dllToWatch) {
        mTemporaryDir = getTemporaryDirectory();
        BUFF_CHECKED_CALL(true, createDirectory(mTemporaryDir));

        BUFF_ASSERT(mDllToLoadAndWatch.getExtension() == "dll");
        mTemporaryDll = mTemporaryDir / "hotswap.dll"_File;

        mOriginalPdb = *mDllToLoadAndWatch.getDirectory() /
                       FilePath(mDllToLoadAndWatch.getFilenameWithoutExtension() + String(".pdb"));
        mTemporaryPdb = mTemporaryDir / "hotswap.pdb"_File;
    }
    ~HotswapHost() {
        cleanPreviousLoad();
        BUFF_CHECKED_CALL(true, removeDirectory(mTemporaryDir));
    }

    Polymorphic* reload() {
        cleanPreviousLoad();
        BUFF_ASSERT(!fileExists(mTemporaryDll));

        // This is pretty fugly, but otherwise we get some "sharing violation 32" error in copy_file: "can't
        // modify files that are currently executing" - even though this file is never executed, it is always
        // copied before executing. Also, we are not changing the file. Only copying it.
        while (true) {
            try {
                BUFF_ASSERT(fileExists(mDllToLoadAndWatch));
                if (copyFile(mDllToLoadAndWatch, mTemporaryDll)) {
                    break;
                }
            } catch (std::exception& ex) {
                std::cerr << "[HOT RELOAD] copyFile failed: " << ex.what() << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }
        BUFF_CHECKED_CALL(true, copyFile(mOriginalPdb, mTemporaryPdb));

        mLoadedDll = LoadLibraryW(mTemporaryDll.getNative().asWString().c_str());
        if (!mLoadedDll) {
            throw Exception("Cannot load DLL!");
        }
        const HotswapInterfaceFn loadedFn =
            reinterpret_cast<HotswapInterfaceFn>(GetProcAddress(mLoadedDll, "getHotswappableInterface"));
        if (!loadedFn) {
            throw Exception("Cannot get getHotswappableInterface function!");
        }
        Polymorphic* hotswappableInterface =
            loadedFn(/*mLastState ? Optional<ArrayView<const std::byte>>(*mLastState)
                               : Optional<ArrayView<const std::byte>>()*/);
        if (!hotswappableInterface) {
            throw Exception("Nullptr Hotswappable!");
        }

        mDirWatcher = makeAutoPtr<DirectoryModificationWatcher>(
            *mDllToLoadAndWatch.getDirectory(),
            [&](const FilePath& path) {
                if (path.getNative() == mDllToLoadAndWatch.getFilenameWithExtension()) {
                    mDllChangeCallback();
                }
            });
        return hotswappableInterface;
    }

private:
    void cleanPreviousLoad() {
        mDirWatcher.deleteResource();
        if (mLoadedDll) {
            BUFF_CHECKED_CALL(TRUE, FreeLibrary(mLoadedDll));
        }
        if (fileExists(mTemporaryDll)) {
#if 1
            // This either flushes the filesystem caches or stalls long enough so we no longer get bogus locks
            // on the files we want to remove...
            ShellExecuteA(nullptr, "open", "powershell", "Write-VolumeCache C", nullptr, SW_SHOWMINNOACTIVE);
            BUFF_CHECKED_CALL(true, removeFile(mTemporaryDll));
            BUFF_CHECKED_CALL(true, removeFile(mTemporaryPdb));
#else
            // No idea why but the above sometimes fails...
            try {
                if (removeFile(mTemporaryDll) && removeFile(mTemporaryPdb)) {
                    return;
                }
            } catch (...) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
#endif
        }
    }
};

BUFF_NAMESPACE_END
