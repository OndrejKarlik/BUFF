#pragma once
#include "Lib/Bootstrap.h"
#include <thread>

BUFF_NAMESPACE_BEGIN

bool isMainThread();

/// Simple wrapper around std::thread, adding std::jthread functionality. This is necessary, because
/// emscripten clang does not support std::jthread.
class Thread : public NoncopyableMovable {
    std::thread mImpl;

public:
    template <typename... TArgs>
    explicit Thread(TArgs&&... args)
        : mImpl(std::forward<TArgs>(args)...) {}

    Thread(Thread&& other)            = default;
    Thread& operator=(Thread&& other) = default;

    ~Thread() {
        if (mImpl.joinable()) {
            mImpl.join();
        }
    }

    void detach() {
        mImpl.detach();
    }

    bool isJoinable() const {
        return mImpl.joinable();
    }
};

template <typename TMutex>
class ScopedLock {
    TMutex* mMutex;

public:
    explicit ScopedLock(TMutex& mutex)
        : mMutex(&mutex) {
        mMutex->lock();
    }
    ~ScopedLock() {
        if (mMutex) {
            mMutex->unlock();
        }
    }
    void unlock() {
        BUFF_ASSERT(mMutex);
        mMutex->unlock();
        mMutex = nullptr;
    }
};

BUFF_NAMESPACE_END
