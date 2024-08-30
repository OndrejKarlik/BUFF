#pragma once
#include "Lib/Assert.h"
#include "Lib/Bootstrap.h"
#include "Lib/containers/ArrayView.h"
#include "Lib/Exception.h"
#include <doctest/doctest.h>
#include <mutex>
#ifndef __EMSCRIPTEN__
#    include "LibWindows/AssertHandler.h"
#    include <fcntl.h>
#else
#    include "LibEmscripten/LibEmscripten.h"
#endif

BUFF_NAMESPACE_BEGIN

namespace Doctest {

struct MyReporter final : doctest::IReporter {
    std::ostream&                  output;
    const doctest::ContextOptions& opt;
    const doctest::TestCaseData*   tc = nullptr;
    std::mutex                     mutex;

    explicit MyReporter(const doctest::ContextOptions& in)
        : output(*in.cout)
        , opt(in) {}

    virtual void test_case_start(const doctest::TestCaseData& in) override {
        output << "Test case " << in.m_name;
        tc = &in;
    }

    virtual void test_case_end(const doctest::CurrentTestCaseStats& in) override {
        const bool fail = in.failure_flags != 0;
        output << (fail ? "FAIL" : "OK") << std::endl;
    }

    virtual void report_query(const doctest::QueryData& BUFF_UNUSED(in)) override {}

    virtual void test_run_start() override {}

    virtual void test_run_end(const doctest::TestRunStats& BUFF_UNUSED(in)) override {}

    virtual void test_case_reenter(const doctest::TestCaseData& BUFF_UNUSED(in)) override {}

    virtual void test_case_exception(const doctest::TestCaseException& BUFF_UNUSED(in)) override {}

    virtual void subcase_start(const doctest::SubcaseSignature& BUFF_UNUSED(in)) override {}

    virtual void subcase_end() override {}

    virtual void log_assert(const doctest::AssertData& BUFF_UNUSED(in)) override {}

    virtual void log_message(const doctest::MessageData& BUFF_UNUSED(in)) override {}

    virtual void test_case_skipped(const doctest::TestCaseData& BUFF_UNUSED(in)) override {}
};

#ifdef DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#    ifdef DOCTEST_REGISTERED
#        error use this only in a single file!
#    endif
#    define DOCTEST_REGISTERED
REGISTER_REPORTER("myOutput", 1, MyReporter);
#endif

#define DOCTEST_VALUE_PARAMETERIZED_DATA(variable, options)                                                  \
    auto                                            optionsCopy_ = options;                                  \
    std::decay_t<decltype(*(optionsCopy_).begin())> variable;                                                \
    static size_t                                   doctestSubcaseIndex_ = 0;                                \
    std::ranges::for_each(optionsCopy_, [&](const auto& in) {                                                \
        const doctest::String name_ = doctest::String(#options "[") +                                        \
                                      doctest::toString(doctestSubcaseIndex_++) +                            \
                                      "]: " + doctest::toString(in);                                         \
        DOCTEST_SUBCASE(name_) {                                                                             \
            CAPTURE(in);                                                                                     \
            (variable) = in;                                                                                 \
        }                                                                                                    \
    });                                                                                                      \
    doctestSubcaseIndex_ = 0

template <IterableContainer T1, IterableContainer T2>
void checkArraysEq(T1&& referenceIn, T2&& valuesIn) {
    using T             = std::decay_t<decltype(*referenceIn.begin())>;
    ArrayView reference = ArrayView<const T>(referenceIn);
    ArrayView curValues = ArrayView<const T>(valuesIn);
    CAPTURE(reference);
    CAPTURE(curValues); // Made on purpose to have the same length - so the printouts line up nicely
    // std::cout << reference;
    REQUIRE(reference.size() == curValues.size());
    for (int i = 0; i < reference.size(); ++i) {
        CAPTURE(i);
        CHECK(reference[i] == curValues[i]);
    }
}

class TestAssertException final : public std::exception {
    AssertArguments mArgs;
    String          mArgsAsString;

public:
    explicit TestAssertException(const AssertArguments& args)
        : mArgs(args) {
        mArgsAsString = String("ASSERT FAILED: ") + mArgs.condition + "\nFile: " + mArgs.filename +
                        "\nLine: " + toStr(mArgs.lineNumber) + "\nArguments:\n" + mArgs.arguments;
    }

    virtual const char* what() const noexcept override {
        return mArgsAsString.asCString();
    }
};

#if BUFF_DEBUG == 1
// TODO: does not reset the assert handler properly?
#    define CHECK_ASSERT(...)                                                                                \
        {                                                                                                    \
            auto    previous_ = getCurrentAssertHandler();                                                   \
            Finally finally_([previous_]() { setAssertHandler(previous_); });                                \
            setAssertHandler([](const AssertArguments& args) { throw Doctest::TestAssertException(args); }); \
            BUFF_DISABLE_MSVC_WARNING_BEGIN(4834)                                                            \
            BUFF_DISABLE_CLANG_WARNING_BEGIN("-Wunused-result")                                              \
            BUFF_DISABLE_CLANG_WARNING_BEGIN("-Wunused-comparison")                                          \
            CHECK_THROWS_AS(__VA_ARGS__, Doctest::TestAssertException);                                      \
            BUFF_DISABLE_CLANG_WARNING_END()                                                                 \
            BUFF_DISABLE_CLANG_WARNING_END() BUFF_DISABLE_MSVC_WARNING_END()                                 \
        }                                                                                                    \
        static_assert(true) /* force semicolon */
#else
#    define CHECK_ASSERT(...) static_assert(true) /* force semicolon */
#endif

} // namespace Doctest

inline int testsMain(int argc, char** argv) {
    using namespace Buff;

    setAssertHandler([](const AssertArguments& args) {
        if (isDebuggerPresent()) {
            debuggingAssertHandler(args);
        } else {
            throw Exception(args.getMessage());
        }
    });

#ifndef __EMSCRIPTEN__
    BUFF_CHECKED_CALL(TRUE, SetConsoleOutputCP(65001));
#endif

    doctest::Context context;
    context.applyCommandLine(argc, argv);

    const int res = context.run(); // run
    if (isDebuggerPresent()) {
        BUFF_ASSERT(!res, "Tests failed");
    }
    return res;
}

// ReSharper disable once CppInconsistentNaming
inline void CHECK_STREQ(const StringView actual, const StringView reference) {
    if (actual == reference) {
        return;
    }
    static constexpr auto RED        = "\033[41m";
    static constexpr auto GREEN      = "\033[42m";
    static constexpr auto BLUE       = "\033[44m";
    static constexpr auto NORMAL     = "\033[0m";
    auto                  getColored = [&](String text, const StringView colorTag) {
        // We need to break coloring at line breaks, otherwise the entire rest of the line would be colored
        text.replaceAll("\n", NORMAL + "\n"_S + colorTag + "\xE2\xA4\xB6"_S);
        return colorTag + text + NORMAL;
    };

    const LevenshteinDistance distance  = getLevenshteinDistance(actual, reference);
    String                    aOut      = "\nACTUAL: ";
    String                    bOut      = "\nREF:    ";
    String                    diff      = "\nDIFF:   ";
    int                       aPtr      = 0;
    String                    aModified = actual;
    for (const auto& change : distance.changes) {
        // if (change.where - aPtr > 0) { // TODO: try remove condition
        const auto unmodified = aModified.getSubstring(aPtr, change.where - aPtr);
        diff << unmodified;
        aOut << unmodified;
        bOut << unmodified;
        aPtr = change.where;
        //}
        switch (change.type) {
        case LevenshteinDistance::INSERTION: {
            const StringView inserted = reference.getSubstring(change.from, change.count);
            diff << getColored(inserted, GREEN);
            aOut << std::string(inserted.size(), ' ');
            bOut << getColored(inserted, GREEN);
            aPtr += change.count;
        } break;
        case LevenshteinDistance::DELETION: {
            const StringView deleted = aModified.getSubstring(change.where, change.count);
            diff << getColored(deleted, RED);
            aOut << getColored(deleted, RED);
            bOut << std::string(deleted.size(), ' ');
        } break;
        case LevenshteinDistance::SUBSTITUTION: {
            const StringView substituted = reference.getSubstring(change.from, change.count);
            diff << getColored(substituted, BLUE);
            aOut << getColored(aModified.getSubstring(change.where, change.count), RED);
            bOut << getColored(substituted, GREEN);
            aPtr += change.count;
        } break;
        default:
            BUFF_STOP;
        }
        change.applyToFrom(aModified, reference);
    }
    const StringView rest = aModified.getSubstring(aPtr);
    diff << rest;
    aOut << rest;
    bOut << rest;
    INFO((aOut + bOut + diff));
    FAIL("Strings are not equal, Levenshtein distance is: " << distance.distance);
}

BUFF_NAMESPACE_END

// ReSharper disable once CppInconsistentNaming
namespace doctest {

template <class T>
struct StringMaker<Buff::Array<T>> {
    static String convert(const Buff::Array<T>& value) {
        String res = "Array[";
        res += toString(value.size());
        res += "]{";
        for (auto& i : value) {
            res += " ";
            res += toString(i);
        }
        res += " }";
        return res;
    }
};

template <class T>
struct StringMaker<Buff::Optional<T>> {
    static String convert(const Buff::Optional<T>& value) {
        if (value) {
            return "Optional(" + toString(*value) + ")";
        } else {
            return "Optional()";
        }
    }
};

} // namespace doctest
