#include "Lib/ThreadPool.h"
#include "Lib/AutoPtr.h"
#include "Lib/Bootstrap.h"
#include "Lib/containers/StableArray.h"
#include "Lib/Function.h"
#include "Lib/Thread.h"
#include "Lib/Tracing.h"
#include <functional>
#include <mutex>
#include <semaphore>
#include <thread>

BUFF_NAMESPACE_BEGIN

// ===========================================================================================================
// ThreadPool
// ===========================================================================================================

struct ThreadPool::Impl {
    Function<void(int)> setThreadName;
    StableArray<Thread> threads {{.granularity = 32}};

    int parallelThreadLimit;

    std::counting_semaphore<128> runningThreads {0};
    std::atomic_int              finishedThreads {0};
    std::binary_semaphore        blockingTaskFinished {0};

    std::atomic<bool> shuttingDown = false;

    /// Params are threadId, workId
    Function<void(int, int64)> currentFunctor;
    int64                      from = -1;
    int64                      to   = -1;
    std::atomic<int64>         current;

    void threadFunc(const int64 threadIndex) {
        setThreadName(int(threads.size()));
        while (!shuttingDown) {
            runningThreads.acquire();
            while (true) {
                const int64 i = current++;
                if (i >= to) {
                    const int value = ++finishedThreads;
                    if (value == threads.size()) {
                        blockingTaskFinished.release();
                    }
                    break;
                }
                currentFunctor(int(threadIndex), i);
            }
        }
    }

    void addThread() {
        threads.pushBack(Thread([this](const int64 index) { threadFunc(index); }, threads.size()));
    }
};

ThreadPool::ThreadPool(Function<void(int)> setThreadName, const int maxNumThreads)
    : mImpl(ALLOCATE_DEFAULT_CONSTRUCTED) {
    mImpl->setThreadName = std::move(setThreadName);
    BUFF_ASSERT(maxNumThreads < 128 && maxNumThreads > 1);
    mImpl->parallelThreadLimit = maxNumThreads;
}

ThreadPool::~ThreadPool() {
    BUFF_ASSERT(!mImpl->currentFunctor);
    mImpl->shuttingDown = true;
    mImpl->runningThreads.release(mImpl->threads.size());
}

void ThreadPool::parallelForBlocking(const int64 from, const int64 to, Function<void(int, int64)> functor) {
    const int64 parallelism = to - from;
    while (mImpl->threads.size() < min<int64>(mImpl->parallelThreadLimit, parallelism)) {
        mImpl->addThread();
    }
    mImpl->from            = from;
    mImpl->to              = to;
    mImpl->current         = 0;
    mImpl->currentFunctor  = std::move(functor);
    mImpl->finishedThreads = 0;
    mImpl->runningThreads.release(mImpl->threads.size());
    mImpl->blockingTaskFinished.acquire();
    mImpl->currentFunctor = nullptr;
}

// ===========================================================================================================
// ThreadTaskPool
// ===========================================================================================================

struct ThreadTaskPool::Impl {
    Function<void(int)> setThreadName;
    std::mutex          runThreadMutex;
    StableArray<Thread> threads {{.granularity = 8}};

    Function<void()> nextTask;

    std::atomic<bool> shuttingDown = false;

    /// Releases a single thread to run
    std::binary_semaphore threadLauncher {0};

    /// Guard for previous thread picking up its work before we can submit a next one
    std::binary_semaphore nextTaskConsumed {1};

    /// The value might be lower than reality, which is not a big issue. It would only mean we create more
    /// threads than we really need when we encounter an edge condition.
    std::atomic_int approxThreadsWaiting {0};

    void threadFunc() {
        setThreadName(int(threads.size()));
        while (!shuttingDown) {
            ++approxThreadsWaiting;
            threadLauncher.acquire();
            --approxThreadsWaiting; // This is synchronized by threadPickedUpTask

            if (shuttingDown) {
                threadLauncher.release(); // Release the next thread in row
                return;
            }
            const auto fn = std::move(nextTask);
            nextTaskConsumed.release();
            BUFF_ASSERT(fn);
            fn();
        }
    }

    void addThread() {
        threads.pushBack(Thread([&]() { threadFunc(); }));
    }
};

ThreadTaskPool::ThreadTaskPool(Function<void(int)> setThreadName)
    : mImpl(ALLOCATE_DEFAULT_CONSTRUCTED) {
    mImpl->setThreadName = std::move(setThreadName);
}

ThreadTaskPool::~ThreadTaskPool() {
    // BUFF_TRACE_DURATION("ThreadTaskPool::~ThreadTaskPool");

    // Just to be sure somebody is not touching runThreadTask
    const ScopedLock lock(mImpl->runThreadMutex);

    // Make sure that all threads have picked up their tasks and will finish them
    mImpl->nextTaskConsumed.acquire();

    // Just one thread is enough - it will release the semaphore for the next one
    mImpl->shuttingDown = true;
    mImpl->threadLauncher.release();
}

void ThreadTaskPool::runThreadTask(Function<void()> functor) {
    const ScopedLock lock(mImpl->runThreadMutex);
    mImpl->nextTaskConsumed.acquire(); // Make sure we can access the nextTask variable

    // See if there is an immediately available thread. If not, add a new one
    if (mImpl->approxThreadsWaiting == 0) {
        mImpl->addThread();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    mImpl->nextTask = std::move(functor);
    // std::cout << ("Waking up work thread" + toStr(runCounter++) + "\n");
    mImpl->threadLauncher.release();
}

BUFF_NAMESPACE_END
