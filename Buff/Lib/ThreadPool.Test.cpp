#include "Lib/ThreadPool.h"
#include "Lib/Bootstrap.Test.h"
#include "Lib/containers/Array.h"
#include "Lib/Function.h"
#include "Lib/Time.h"
#include <mutex>
#include <semaphore>

BUFF_NAMESPACE_BEGIN

static void setTestThreadName(const int id) {
#ifndef __EMSCRIPTEN__
    setThreadName(GetCurrentThread(), "_Test_" + toStr(id));
#else
    (void)id; // TODO: Is this implementable in emscripten?
#endif
}

TEST_CASE("ThreadPool default construct") {
    ThreadPool threadPool(setTestThreadName);
}

TEST_CASE("ThreadPool") {
    ThreadPool threadPool(setTestThreadName);
    Array<int> array(128);
    std::ranges::fill(array, 0);
    threadPool.parallelForBlocking(0, array.size(), [&](const int BUFF_UNUSED(threadId), const int64 workId) {
        array[workId]++;
    });
    for (auto& i : array) {
        REQUIRE(i == 1);
    }
}

TEST_CASE("ThreadTaskPool default construct") {
    ThreadTaskPool threadPool(setTestThreadName);
}

struct ThreadTestOption {
    int                  taskLength;
    int                  submitDelay;
    bool                 checkDestructor;
    int                  iterations = 128;
    friend std::ostream& operator<<(std::ostream& os, const ThreadTestOption& option) {
        os << option.taskLength << " " << option.submitDelay << " " << option.checkDestructor << " "
           << option.iterations;
        return os;
    }
};

TEST_CASE("ThreadTaskPool") {
    // BUFF_TRACE_DURATION("ThreadTaskPool test");
    DOCTEST_VALUE_PARAMETERIZED_DATA(options,
                                     (Array<ThreadTestOption> {
                                         {-1, -1, false, 128},
                                         {0,  0,  false, 128},
                                         {-1, 0,  false, 128},
                                         {1,  0,  false, 32 },
                                         {1,  1,  false, 8  },
                                         {1,  1,  false, 1  },
                                         //{5,  1,  false},
                                         //{1,  5,  false},
                                         {-1, -1, true,  128},
                                         {0,  0,  true,  128},
                                         {-1, 0,  true,  128},
                                         {1,  0,  true,  32 },
                                         {1,  1,  true,  8  },
                                         {1,  1,  true,  1  },
                                         //{5,  1,  true },
                                         //{1,  5,  true }
    }));

    Array<int> array(options.iterations);
    auto       check = [&]() {
        for (auto& i : iterate(array)) {
            REQUIRE(i.value() == i.index());
        }
    };

    {
        std::atomic_int       opsDone = 0;
        std::binary_semaphore semaphore {0};
        auto                  task = [&](const int index) {
            if (options.taskLength >= 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(options.taskLength));
            }

            array[index] = index;
            if (++opsDone == options.iterations) {
                semaphore.release();
            }
        };

        ThreadTaskPool pool(setTestThreadName);
        for (int i : range(options.iterations)) {
            if (options.submitDelay >= 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(options.submitDelay));
            }
            pool.runThreadTask([i, task]() { task(i); });
        }
        if (!options.checkDestructor) {
            semaphore.acquire();
            check();
        }
    }
    if (options.checkDestructor) {
        check();
    }
}

BUFF_NAMESPACE_END
