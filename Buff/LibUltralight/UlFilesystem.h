#pragma once
#include "LibUltralight/LibUltralight.h"
#include "LibUltralight/UlLogger.h"
#include "Lib/Filesystem.h"
#include "Lib/meta/MetaStruct.h"
#include "Lib/Path.h"
#include "Lib/SharedPtr.h"
#include <AppCore/Platform.h>
#include <Ultralight/Buffer.h>
#include <Ultralight/platform/Logger.h>
#include <Ultralight/String.h>

BUFF_NAMESPACE_BEGIN

constexpr bool LOG_FILESYSTEM = true;

using MemoryFile = Array<std::byte>;

inline String normalizeVirtualFilePath(const FilePath& path) {
    return path.getNative().getToLower();
}

class UlFilesystem final : public ul::FileSystem {
    DirectoryPath                     mBasePath;
    ul::FileSystem*                   mDefault;
    UlLogger&                         mLogger;
    Map<String, WeakPtr<MemoryFile>>& mVirtualFiles;

    Map<void*, SharedPtr<MemoryFile>> mLockedFiles;

public:
    explicit UlFilesystem(const DirectoryPath&              basePath,
                          UlLogger&                         logger,
                          Map<String, WeakPtr<MemoryFile>>& virtualFiles)
        : mBasePath(basePath)
        , mLogger(logger)
        , mVirtualFiles(virtualFiles) {
        mDefault = ul::GetPlatformFileSystem(toUl(basePath.getNative()));
    }
    virtual bool FileExists(const ul::String& filePath) override {
        bool res;
        if (const Optional<String> virtualFile = getVirtualFilepath(filePath)) {
            res = mVirtualFiles.contains(*virtualFile);
        } else {
            res = mDefault->FileExists(filePath);
        }
        if (!res && fromUl(filePath) == "resources/icudt67l.dat") {
            // This file is requested early in the initialization process and if it does not exist, the
            // application exits immediately outside our control
            displayMessageWindow(
                "Error",
                "Required asset file missing: '" + fromUl(filePath) +
                    "'. The application will now close. Please check it is installed/copied correctly.");
        }
        if constexpr (LOG_FILESYSTEM) {
            mLogger.LogMessage(ul::LogLevel::Info,
                               ul::String("FILESYSTEM: FileExists[") + (res ? "true" : "false") +
                                   "]: " + filePath);
        }
        return res;
    }
    virtual ul::String GetFileMimeType(const ul::String& filePath) override {
        // There is a bug/design flaw in default ultralight implementation of this method on Windows, where
        // MIME is read from system registry (HKEY_CLASSES_ROOT/.[ext]/Content Type). We have encountered
        // cases where this key did not exist on some machines, either because the Windows version was too old
        // or it was a system where developer tools were never installed. This caused returning the default
        // "application/unknown" for .js files, which makes WebKit reject loading them as modules.
        // Therefore, we need our own implementation for at least some of the types.
        //
        // The wrong implementation is in "AppCore\src\win\FileSystemWin.cpp, and we are using portions of
        // Linux implementation from AppCore\src\linux\FileSystemBasic.cpp

        static const Map<StringView, StringView> MIME_MAP =
            Map<StringView, StringView>(std::initializer_list<std::pair<StringView, StringView>> {
                {"js"_Sv,   "application/javascript"_Sv},
                {"json"_Sv, "application/json"_Sv      },
                {"jsx"_Sv,  "text/jscript"_Sv          },

                {"htm"_Sv,  "text/html"_Sv             },
                {"html"_Sv, "text/html"_Sv             },

                {"css"_Sv,  "text/css"_Sv              },

                {"gif"_Sv,  "image/gif"_Sv             },
                {"jpe"_Sv,  "image/jpeg"_Sv            },
                {"jpeg"_Sv, "image/jpeg"_Sv            },
                {"jpg"_Sv,  "image/jpeg"_Sv            },
                {"ico"_Sv,  "image/x-icon"_Sv          },
                {"dib"_Sv,  "image/bmp"_Sv             },
                {"png"_Sv,  "image/png"_Sv             },
                {"webm"_Sv, "video/webm"_Sv            },
                {"webp"_Sv, "image/webp"_Sv            },
                {"svg"_Sv,  "image/svg+xml"_Sv         },

                {"zip"_Sv,  "application/zip"_Sv       },

                {"xml"_Sv,  "text/xml"_Sv              },
                {"txt"_Sv,  "text/plain"_Sv            },
                {"pdf"_Sv,  "application/pdf"_Sv       },
        });
        ul::String result;

        const String extension = FilePath(fromUl(filePath)).getExtension().valueOr("").getToLower();
        if (const auto& found = MIME_MAP.find(extension)) {
            result = toUl(*found);
        } else {
            BUFF_ASSERT(false, "Unknown MIME type for extension: " + extension);
            if (const Optional<String> virtualFile = getVirtualFilepath(filePath)) {
                result = "application/unknown"; // TODO
            } else {
                result = mDefault->GetFileMimeType(filePath);
            }
        }
        if constexpr (LOG_FILESYSTEM) {
            mLogger.LogMessage(ul::LogLevel::Info,
                               "FILESYSTEM: GetFileMimeType: " + filePath + " -> " + result);
        }
        return result;
    }
    virtual ul::String GetFileCharset(const ul::String& filePath) override {
        ul::String result;
        if (const Optional<String> virtualFile = getVirtualFilepath(filePath)) {
            result = "utf-8"; // TODO
        } else {
            result = mDefault->GetFileCharset(filePath);
        }
        if constexpr (LOG_FILESYSTEM) {
            mLogger.LogMessage(ul::LogLevel::Info,
                               "FILESYSTEM: GetFileCharset: " + filePath + " -> " + result);
        }
        return result;
    }
    virtual ul::RefPtr<ul::Buffer> OpenFile(const ul::String& filePath) override {
        ul::RefPtr<ul::Buffer> result;
        if (const Optional<String> virtualFile = getVirtualFilepath(filePath)) {
            if (const auto* it = mVirtualFiles.find(*virtualFile)) {
                if (SharedPtr<MemoryFile> locked = it->lock()) {
                    BUFF_ASSERT(!mLockedFiles.contains(locked->data()));
                    mLockedFiles[locked->data()] = locked;
                    result                       = ul::Buffer::Create(
                        locked->data(),
                        locked->size(),
                        this,
                        [](void* filesystem, void* buffer) {
                            static_cast<UlFilesystem*>(filesystem)->mLockedFiles.erase(buffer);
                        });
                }
            }
        } else {
            // Custom implementation so we do not lock all the files for duration of the application run
            if (Optional<Array<std::byte>> content = readBinaryFile(mBasePath / FilePath(fromUl(filePath)))) {
                Array<std::byte>* heapArray = new Array(std::move(*content));
                result                      = ul::Buffer::Create(heapArray->data(),
                                            heapArray->size(),
                                            heapArray,
                                            [](void* userData, void* BUFF_UNUSED(data)) {
                                                Array<std::byte>* heapArray =
                                                    static_cast<Array<std::byte>*>(userData);
                                                delete heapArray;
                                            });
                // TODO: Multiple files are allocated but only one ever freed? Investigate...
            } else {
                BUFF_ASSERT(false, "File not found: " + fromUl(filePath));
                result = nullptr;
            }
        }
        if constexpr (LOG_FILESYSTEM) {
            mLogger.LogMessage(ul::LogLevel::Info,
                               "FILESYSTEM: OpenFile: " + filePath + " -> " + toUl(toStr(result.get())));
        }
        return result;
    }

private:
    static Optional<String> getVirtualFilepath(const ul::String& filePath) {
        const String native = fromUl(filePath);
        if (native.startsWith(VIRTUAL_FILE_PREFIX)) {
            return normalizeVirtualFilePath(FilePath(native.getSubstring(VIRTUAL_FILE_PREFIX.size())));
        } else {
            return {};
        }
    }
};

BUFF_NAMESPACE_END
