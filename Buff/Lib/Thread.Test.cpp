#include "Lib/Thread.h"
#include "Lib/Bootstrap.Test.h"
#include <thread>

BUFF_NAMESPACE_BEGIN

TEST_CASE("isMainThread") {
    CHECK(isMainThread());

    std::thread([]() { CHECK(!isMainThread()); }).join();
}

BUFF_NAMESPACE_END
