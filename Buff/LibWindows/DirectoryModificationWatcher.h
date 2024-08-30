#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/Function.h"
#include "Lib/Path.h"
#include "Lib/Thread.h"
#include <mutex>
#include <thread>
#include <Windows.h>

BUFF_NAMESPACE_BEGIN

class DirectoryModificationWatcher : public Noncopyable {
    Function<void(const FilePath&)> mCallback;
    HANDLE                          mDir;
    Thread                          mThread;
    std::atomic_bool                mClosing = false;
    std::mutex                      mMutex;

public:
    DirectoryModificationWatcher(const DirectoryPath& directory, Function<void(const FilePath&)> callback);

    ~DirectoryModificationWatcher();
};

BUFF_NAMESPACE_END
