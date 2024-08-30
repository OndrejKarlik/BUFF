#include "LibWindows/DirectoryModificationWatcher.h"
#include "LibWindows/Platform.h"
#include "Lib/Thread.h"

BUFF_NAMESPACE_BEGIN

DirectoryModificationWatcher::DirectoryModificationWatcher(const DirectoryPath&            directory,
                                                           Function<void(const FilePath&)> callback)
    : mCallback(std::move(callback)) {
    // Implemented according to notes from this article, but significantly simplified
    // https://qualapps.blogspot.com/2010/05/understanding-readdirectorychangesw_19.html
    const auto dirName = directory.getGeneric().asWString();
    mDir               = CreateFile(
        dirName.c_str(),                    // pointer to the file name
        FILE_LIST_DIRECTORY,                // access (read/write) mode
        FILE_SHARE_READ | FILE_SHARE_WRITE, //| FILE_SHARE_DELETE, <-- removing FILE_SHARE_DELETE prevents
                                            // the user or someone else from renaming or deleting the
                                            // watched directory. This is a good thing to prevent.
        nullptr,                    // security descriptor
        OPEN_EXISTING,              // how to create
        FILE_FLAG_BACKUP_SEMANTICS, /* MSDN: "You must set this flag to obtain a handle to a directory."*/
                                    // file attributes
        nullptr);                   // file with attributes to copy

    mThread = Thread([&]() {
        setThreadName(GetCurrentThread(), "!"_S + BUFF_LIBRARY_NAME + "_DirectoryModificationWatcher thread");
        StaticArray<DWORD, sizeof(FILE_NOTIFY_INFORMATION) * 1024 / sizeof(DWORD)> buffer;
        std::ranges::fill(buffer, 0);
        constexpr auto FLAGS = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
                               FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE |
                               FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_LAST_ACCESS |
                               FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SECURITY;
        while (true) {
            DWORD bytesReturned = 0;

            [[maybe_unused]] const BOOL res = ReadDirectoryChangesW(mDir,           // handle to directory
                                                                    &buffer,        // read results buffer
                                                                    sizeof(buffer), // length of buffer
                                                                    FALSE,          // monitoring option
                                                                    FLAGS,          // filter conditions
                                                                    &bytesReturned, // bytes returned
                                                                    nullptr,        // overlapped buffer
                                                                    nullptr);       // completion routine
            BUFF_ASSERT(mClosing || res); // Function cannot fail if we are not closing yet.

            if (mClosing) {
                mThread.detach();
                break;
            }
            int offset = 0;
            while (true) {
                FILE_NOTIFY_INFORMATION& notify = *reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                    reinterpret_cast<std::byte*>(buffer.data()) + offset);
                std::wstring tmp(notify.FileName, notify.FileName + notify.FileNameLength);
                // For some reason we get 16 as FileNameLength even when the filename is just few bytes
                // and the rest is filled with nulls...
                for (int i = 0; i < int(tmp.size()); ++i) {
                    if (tmp[i] == L'\0') {
                        tmp.resize(i);
                    }
                }
                mCallback(FilePath(String(tmp)));
                if (notify.NextEntryOffset == 0) {
                    break;
                }
                offset += notify.NextEntryOffset;
            }
        }
    });
}

DirectoryModificationWatcher::~DirectoryModificationWatcher() {
    mClosing = true;
    while (mThread.isJoinable()) {
        // Do not check return value. The worker thread might not have been waiting on ReadDirectoryChangesW
        // at the moment and there does not appear to be an easy way to tell if it is in a thread-safe way. We
        // might try a different approach, e.g. explore OVERLAPPED I/O or send some dummy notification to
        // ReadDirectoryChangesW
        CancelIoEx(mDir, nullptr);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    BUFF_CHECKED_CALL(TRUE, CloseHandle(mDir));
}

BUFF_NAMESPACE_END
