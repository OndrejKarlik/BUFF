#include "LibWindows/DirectoryModificationWatcher.h"
#include "LibWindows/Platform.h"
#include "Lib/Bootstrap.Test.h"
#include "Lib/Filesystem.h"
#include "Lib/Thread.h"
#include "Lib/Time.h"

BUFF_NAMESPACE_BEGIN

struct TestingFunctor {
    std::atomic<int> timesCalled = {0};
    FilePath         lastPath;

    auto getFunctor() {
        return [this](const FilePath& path) {
            CHECK(!isMainThread());
            lastPath = path;
            ++timesCalled;
        };
    }
};

TEST_CASE("DirectoryModificationWatcher.construction_destruction") {
    TestingFunctor functor;
    CHECK_NOTHROW(const DirectoryModificationWatcher watcher("C:/Windows"_Dir, functor.getFunctor()));
    CHECK(int(functor.timesCalled) == 0);
}

TEST_CASE("DirectoryModificationWatcher.observe") {
    static const Array<FilePath> VALUES = {
        "file.txt"_File,
        "some_longer_filename_over_16_bytes.very_long_extension"_File,
    };
    DOCTEST_VALUE_PARAMETERIZED_DATA(filename, VALUES);
    const DirectoryPath dir  = getTemporaryDirectory();
    const FilePath      path = dir / filename;
    CHECK(createDirectory(dir));
    TestingFunctor functor;
    {
        const DirectoryModificationWatcher watcher(dir, functor.getFunctor());
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Give time to watcher to start waiting
        // TODO: better synchronization? That would require some WaitX method used inside the directory
        // watcher.
        CHECK(writeTextFile(path, "aaaa"));
        const Timer timer;
        while (true) {
            if (timer.getElapsed().toMilliseconds() > 2000) {
                CHECK((false && "timeout while waiting for the change notification"));
                break;
            }
            if (functor.timesCalled > 0) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        CHECK(removeFile(path));
    }
    CHECK(functor.lastPath == filename);
    CHECK(removeDirectory(dir));
}

TEST_CASE("DirectoryModificationWatcher.expired") {
    const DirectoryPath dir  = getTemporaryDirectory();
    const FilePath      path = dir / "file.txt"_File;
    CHECK(createDirectory(dir));
    TestingFunctor functor;
    { const DirectoryModificationWatcher watcher(dir, functor.getFunctor()); }
    CHECK(writeTextFile(path, "aaaa"));
    CHECK(removeFile(path));
    CHECK(removeDirectory(dir));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    CHECK(int(functor.timesCalled) == 0);
}

BUFF_NAMESPACE_END
