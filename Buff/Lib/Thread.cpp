#include "Lib/Thread.h"
#include <thread>

BUFF_NAMESPACE_BEGIN

static const std::thread::id MAIN_THREAD_ID = std::this_thread::get_id();

bool isMainThread() {
    return std::this_thread::get_id() == MAIN_THREAD_ID;
}

BUFF_NAMESPACE_END
