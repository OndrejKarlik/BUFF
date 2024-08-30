#include "Lib/Filesystem.h"
#include "Lib/Bootstrap.Test.h"
#include "Lib/Path.h"

BUFF_NAMESPACE_BEGIN

// ===========================================================================================================
// Files
// ===========================================================================================================

TEST_CASE("fileExists") {
    CHECK(fileExists("C:/Windows/explorer.exe"_File));
    CHECK(!fileExists("C:/Windows/explorer-foo.exe"_File));
}

TEST_CASE("fileSize") {
    const FilePath filename("filename-with-utf-\x68\xc3\xa1\xc4\x8d\x65\x6b");
    CHECK(writeTextFile(filename, "abcd"));
    const auto size = fileSize(filename);
    REQUIRE(size);
    CHECK(*size == 4);
}

TEST_CASE("fileSize") {
    const FilePath file1("file1.txt");
    const FilePath file2("file2.txt");
    const FilePath file3("file3.txt");
    CHECK(writeTextFile(file1, "abcdef"));
    CHECK(writeTextFile(file2, "abcdef"));
    CHECK(writeTextFile(file3, "ghchij"));

    CHECK(areFilesIdentical(file1, file2));
    CHECK(!areFilesIdentical(file1, file3));
}

// ===========================================================================================================
// Directories
// ===========================================================================================================

TEST_CASE("directoryExists") {
    CHECK(directoryExists("C:/Windows"_Dir));
    CHECK(directoryExists("C:/Windows/"_Dir));
    CHECK(!directoryExists("C:/Windows-foo"_Dir));
    CHECK(!directoryExists("C:/Windows-foo/"_Dir));
}

TEST_CASE("createDirectory/removeDirectory") {
    CHECK(!createDirectory("ABC:/foo"_Dir));
    const auto tempDir = "foo"_Dir;
    CHECK(!directoryExists(tempDir));
    CHECK(createDirectory(tempDir));
    CHECK(directoryExists(tempDir));
    CHECK(createDirectory(tempDir));
    CHECK(removeDirectory(tempDir));
    CHECK(!directoryExists(tempDir));
    CHECK(removeDirectory(tempDir));
}

// ===========================================================================================================
// Enumerating
// ===========================================================================================================

// ===========================================================================================================
// Reading/Writing
// ===========================================================================================================

TEST_CASE("readTextFile") {
    const FilePath path        = getTemporaryFilename();
    const String   fileContent = "a\n"
                                 "b\r\n"
                                 "c\r"
                                 "d\r\n"
                                 "\r\n"
                                 "e\n"
                                 "\n"
                                 "f\r"
                                 "\rg";
    CHECK(writeBinaryFile(path, ArrayView(fileContent.asCString(), fileContent.size()).asBytes()));
    BUFF_ASSERT(fileSize(path) == fileContent.size());
    auto result = readTextFile(path);
    CHECK(result);
    CHECK_STREQ(*result, "a\nb\nc\nd\n\ne\n\nf\n\ng");
    CHECK(removeFile(path));
}

BUFF_NAMESPACE_END
