#include "Lib/Assert.h"
#include "Lib/AutoPtr.h"
#include "Lib/Filesystem.h"
#include "Lib/Path.h"
#include "Lib/SharedPtr.h"
#include "Lib/Time.h"
#ifdef _WIN32
#    include "LibWindows/AssertHandler.h"
#elif defined(__EMSCRIPTEN__)
#    include <iostream>
#endif

BUFF_NAMESPACE_BEGIN

// Just test that everything in natvis works
static void testNatvis() {

    // ReSharper disable CppLocalVariableWithNonTrivialDtorIsNeverUsed
    const String stringEmpty;
    const String string = "foo";

    const AutoPtr<int> autoPtrEmpty;
    const AutoPtr<int> autoPtr = makeAutoPtr<int>(42);

    const SharedPtr<int> sharedPtrEmpty;
    const SharedPtr<int> sharedPtr = makeSharedPtr<int>(42);

    const SharedPtr<const int> constSharedPtrEmpty;
    const SharedPtr<const int> constSharedPtr = makeSharedPtr<int>(42);

    [[maybe_unused]] const Duration duration = Duration::milliseconds(666);

    [[maybe_unused]] constexpr volatile int PLACE_TO_PUT_BREAKPOINT = 0;
    // ReSharper restore CppLocalVariableWithNonTrivialDtorIsNeverUsed
}

static void run() {

    // Generating a lookup table for CP1252 conversion in String.cpp
    if constexpr ((false)) {
        Array<std::byte> arr;
        for (int i = 128; i < 256; ++i) {
            arr.pushBack(static_cast<std::byte>(i));
            arr.pushBack(static_cast<std::byte>(' '));
        }
        BUFF_CHECKED_CALL(true, writeBinaryFile("D:/test.txt"_File, arr));
    }
    testNatvis();
}

BUFF_NAMESPACE_END

int main() {

#ifdef _WIN32
    Buff::setAssertHandler(Buff::debuggingAssertHandler);
#else
    Buff::setAssertHandler([](const Buff::AssertArguments& args) {
        // Basically equal to debuggingAssertHandler, but do not break in debugger
        std::cerr << "ASSERT FAILED: " << args.condition << "\nFile: " << args.filename
                  << "\nLine: " << args.lineNumber << "\nArguments:" << args.arguments << std::endl;
    });
#endif

    Buff::run();
    return 0;
}
