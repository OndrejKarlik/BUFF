#pragma once
#include "Lib/AutoPtr.h"
#include "Lib/Bootstrap.h"
#include <functional>
#include <thread>

BUFF_NAMESPACE_BEGIN

template <typename T>
class Function;

class ThreadPool : public Noncopyable {
    struct Impl;
    AutoPtr<Impl> mImpl;

public:
    /// \param setThreadName
    /// Called from each spawned thread, should set name for the thread for the debugger
    /// \param maxNumThreads
    /// Maximum number of threads that will be used by this threadpool
    explicit ThreadPool(Function<void(int)> setThreadName,
                        int                 maxNumThreads = std::thread::hardware_concurrency());
    ~ThreadPool();

    /// Function uses min(to-from, maxNumThreads) threads to execute the functor. Blocks until done
    void parallelForBlocking(int64 from, int64 to, Function<void(int, int64)> functor);
};

class ThreadTaskPool : public Noncopyable {
    struct Impl;
    AutoPtr<Impl> mImpl;

public:
    /// \param setThreadName
    /// Called from each spawned thread, should set name for the thread for the debugger
    ThreadTaskPool(Function<void(int)> setThreadName);

    /// Blocks until all tasks are finished
    ~ThreadTaskPool();

    /// Is thread safe to use
    void runThreadTask(Function<void()> functor);
};

BUFF_NAMESPACE_END
