#include "LibWindows/Net.h"
#include "Lib/Bootstrap.Test.h"
#include "Lib/Function.h"
#include <semaphore>

BUFF_NAMESPACE_BEGIN

TEST_CASE("urlEncode") {
    CHECK_STREQ(urlEncode(""), "");
    CHECK_STREQ(urlEncode("a"), "a");
    CHECK_STREQ(urlEncode(" "), "%20");
    CHECK_STREQ(urlEncode(" ", true), "+");
    CHECK_STREQ(urlEncode("abc0123123XYZ987zxy"), "abc0123123XYZ987zxy");
    CHECK_STREQ(urlEncode("a b + cdef"), "a%20b%20%2b%20cdef");
    CHECK_STREQ(urlEncode("a b + cdef", true), "a+b+%2b+cdef");
    CHECK_STREQ(urlEncode("abc/def%ghi%%ijk"), "abc%2fdef%25ghi%25%25ijk");
}

TEST_CASE("httpPostRequest payload") {
    const String           payload = "wharr garbl / % - * /  %% _";
    const Expected<String> res     = httpPostRequest("httpbin.org/post", HttpPostPayload(payload));
    CHECK(res);
    INFO(res);
    CHECK(res->contains(payload));
}

TEST_CASE("httpPostRequest params") {
    Array<std::pair<String, String>> params = {
        {"par am", "val/ue"}
    };
    const Expected<String> res = httpPostRequest("httpbin.org/post", HttpPostPayload(params));
    CHECK(res);
    INFO(res);
    CHECK(res->contains("par%20am=val%2fue"));
}

TEST_CASE("httpPostRequest async") {
    Array<std::pair<String, String>> params = {
        {"par am", "val/ue"}
    };
    std::binary_semaphore semaphore(0);
    int                   numCalled = 0;
    const auto            functor   = [&](Expected<String> res) {
        CHECK(numCalled++ == 0);
        INFO(res);
        CHECK(res);
        CHECK(res->contains("par%20am=val%2fue"));
        semaphore.release();
    };
    httpPostRequestAsync(functor, "httpbin.org/post", HttpPostPayload(params));
    semaphore.acquire();
}

BUFF_NAMESPACE_END
