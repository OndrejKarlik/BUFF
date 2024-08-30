#include "Lib/Cryptography.h"
#include "Lib/Bootstrap.Test.h"
#include "Lib/containers/Array.h"

BUFF_NAMESPACE_BEGIN

// ===========================================================================================================
// Base64
// ===========================================================================================================

TEST_CASE("Cryptography.toBase64") {
    CHECK(toBase64(String()) == "");
    CHECK(toBase64("a") == "YQ==");
    CHECK(toBase64("ab") == "YWI=");
    CHECK(toBase64("abc") == "YWJj");
    const String hw = String("Hello world!");
    CHECK(toBase64(ArrayView(reinterpret_cast<const std::byte*>(hw.asCString()), hw.size())) ==
          "SGVsbG8gd29ybGQh");
    CHECK(toBase64(hw) == "SGVsbG8gd29ybGQh");
}

TEST_CASE("Cryptography.fromBase64") {
    auto getRef = [](const StringView in) {
        Array<std::byte> res;
        for (const char i : in) {
            res.pushBack(std::byte(i));
        }
        return res;
    };

    CHECK(fromBase64(String()) == Array<std::byte>());
    CHECK(!fromBase64("a"));
    CHECK(fromBase64("YQ==") == getRef("a"));
    CHECK(fromBase64("YWI=") == getRef("ab"));
    CHECK(fromBase64("YWJj") == getRef("abc"));
    CHECK(fromBase64("SGVsbG8gd29ybGQh") == getRef("Hello world!"));
}

// ===========================================================================================================
// MD5
// ===========================================================================================================

TEST_CASE("Cryptography.md5") {
    CHECK(getMd5Ascii("a") == "0cc175b9c0f1b6a831c399e269772661");
    CHECK(getMd5Ascii("Hello world!") == "86fb269d190d2c85f6e0468ceca42a20");
    CHECK(
        getMd5Ascii(
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Quisque semper aliquam malesuada. "
            "Curabitur pellentesque ante dolor, sit amet molestie orci sagittis ac. Pellentesque scelerisque "
            "ipsum metus. Praesent tincidunt est sed leo posuere venenatis. Etiam aliquet vulputate finibus. "
            "Suspendisse id diam nibh. Maecenas iaculis purus eu consequat commodo. Nunc facilisis ipsum "
            "urna, nec porta diam tristique eu. Aliquam gravida libero in aliquam viverra. Quisque facilisis "
            "porttitor ipsum in sodales. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices "
            "posuere cubilia curae; Nam et eleifend lacus. Suspendisse sollicitudin nec magna in porttitor. "
            "Vivamus tristique lacus id turpis sagittis elementum. Donec gravida lorem tellus, sed iaculis "
            "magna pretium a. Proin blandit sapien faucibus efficitur aliquet.") ==
        "7985be5cb8944d8f159ca836f04144dc");
}

BUFF_NAMESPACE_END
