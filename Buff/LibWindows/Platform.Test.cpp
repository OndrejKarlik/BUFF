#include "LibWindows/Platform.h"
#include "Lib/Bootstrap.Test.h"

BUFF_NAMESPACE_BEGIN

TEST_CASE("Platform.tempDirs") {
    CHECK_NOTHROW(getGlobalTempDirectory());
    CHECK_NOTHROW(getTemporaryFilename());
    CHECK_NOTHROW(getTemporaryDirectory());

    CHECK(getGlobalTempDirectory() == getGlobalTempDirectory());
    CHECK(getTemporaryFilename() != getTemporaryFilename());
    CHECK(getTemporaryDirectory() != getTemporaryDirectory());
}

TEST_CASE("Platform.executeProcess") {
    for (const int i : range(3)) {
        Expected<ExecuteProcessResult> result = ExecuteProcessResult {};
        CHECK_NOTHROW(result = executeProcess({
                          .executable = "C:/Windows/System32/cmd.exe"_File,
                          .args       = {"/c", "echo hello world " + toStr(i)}
        }));
        CHECK(result);
        CHECK(result->returnValue == 0);
        CHECK(result->stdOut.getTrimmed() == "hello world " + toStr(i));
        CHECK(result->stdErr == "");
    }
}
TEST_CASE("Platform.executeProcess no redirect") {
    for (const int i : range(3)) {
        Expected<ExecuteProcessResult> result = ExecuteProcessResult {};
        CHECK_NOTHROW(result = executeProcess({
                          .executable             = "C:/Windows/System32/cmd.exe"_File,
                          .args                   = {"/c", "echo hello world " + toStr(i)},
                          .captureStandardOutputs = false,
        }));
        CHECK(result);
        CHECK(result->returnValue == 0);
        CHECK(result->stdOut.isEmpty());
        CHECK(result->stdErr == "");
    }
}

TEST_CASE("Platform.getRefreshRate") {
    const auto displayRate = getRefreshRate();
    CHECK(displayRate);
    CHECK(*displayRate >= 30);
}

TEST_CASE("Platform.isDebuggerPresent") {
    CHECK_NOTHROW(isDebuggerPresent());
}

TEST_CASE("Platform.executeShell") {
    CHECK(executeShell("echo hello world") == 0);
}

TEST_CASE("Platform.getAllDrives") {
    const auto drives = getAllDrives();
    CHECK(drives.notEmpty());
    CHECK((drives.contains('C') || drives.contains('c')));
}

TEST_CASE("Platform.getThisModuleFilename") {
    const auto filename = getThisModuleFilename();
    CHECK(filename.getNative().notEmpty());
    CHECK(filename.getNative().endsWith("LibWindows.Test.exe"));
}

TEST_CASE("Platform.getConsoleSize") {
    const auto size = getConsoleSize();
    CHECK(size.x > 0);
    CHECK(size.y > 0);
}

TEST_CASE("Platform.getRefreshRate") {
    const auto rate = getRefreshRate();
    CHECK(rate);
    CHECK(*rate > 29); // RDesktop running at 32 fps?
    CHECK(*rate < 200);
}

TEST_CASE("Platform.getDriveFreeSpace") {
    CHECK(getDriveFreeSpace('C') > 0);
}

TEST_CASE("Platform.getGlobalTempDirectory") {
    const auto dir = getGlobalTempDirectory();
    CHECK(dir.getNative().notEmpty());
    CHECK(dir.getNative().contains("Temp"));
}

TEST_CASE("Platform.getTemporaryFilename") {
    const auto filename = getTemporaryFilename();
    CHECK(filename.getNative().notEmpty());
    CHECK(filename.getNative().startsWith(getGlobalTempDirectory().getNative()));
    const auto filename2 = getTemporaryFilename();
    CHECK(filename != filename2);
}

TEST_CASE("Platform.getTemporaryDirectory") {
    const auto dir = getTemporaryDirectory();
    CHECK(dir.getNative().notEmpty());
    CHECK(dir.getNative().startsWith(getGlobalTempDirectory().getNative()));
    const auto dir2 = getTemporaryDirectory();
    CHECK(dir != dir2);
}

TEST_CASE("Platform.getRoamingDirectory") {
    const auto dir = getRoamingDirectory();
    CHECK(dir.getNative().notEmpty());
    CHECK(dir.getNative().contains("Roaming"));
}

TEST_CASE("Platform.getDriveName") {
    CHECK_NOTHROW(getDriveName('C'));
    CHECK_ASSERT(getDriveName(0));
}

TEST_CASE("Platform.parseCommandLine") {
    const auto args = parseCommandLine("hello world -flag /flag");
    CHECK(args.size() == 4);
    CHECK(args[0] == "hello");
    CHECK(args[1] == "world");
    CHECK(args[2] == "-flag");
    CHECK(args[3] == "/flag");

    // Complicated example with " handling
    const auto args2 = parseCommandLine(R"(hello "w o r l d" "hello \"world\""  )");
    CHECK(args2.size() == 3);
    CHECK(args2[0] == "hello");
    CHECK(args2[1] == "w o r l d");
    CHECK(args2[2] == R"(hello "world")");
}

TEST_CASE("Platform.ExecuteProcessParams.getCommandLine") {
    ExecuteProcessParams params;
    params.executable               = "foo.exe"_File;
    const Array<Array<String>> DATA = {
        {"hello", "world"},
        {"hello world"},
        {"\"hello world\""},
        {"hello", "world", "hello \"world\""},
        {"hello", "world", "hello \"world\"", R"(hello
                                                 world)"},
        {"hel\"lo", "wor\\ld", "\"", "\"\""},
        {"\"hello\"", "world\\", "\"hello\\\"", "\"hello\\\\\""},
        {"hello\\", "hello\\\"", "hello\\\\\""},
    };
    DOCTEST_VALUE_PARAMETERIZED_DATA(reference, DATA);
    params.args  = reference;
    auto cmdLine = params.getCommandLine();
    CAPTURE(cmdLine);
    Array<String> actual = parseCommandLine(cmdLine);
    CHECK(actual[0] == params.executable.getGeneric());
    actual.eraseByIndex(0);
    CHECK(reference == actual);
}

TEST_CASE("Platform.expandEnvironmentVariables") {
    setEnvironmentVariable("HELLO", "world");
    CHECK(expandEnvironmentVariables("hello world") == "hello world");
    CHECK(expandEnvironmentVariables("%HELLO% world") == "world world");
}

BUFF_NAMESPACE_END
