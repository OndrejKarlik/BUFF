#include "Lib/Path.h"
#include "Lib/Bootstrap.Test.h"
#include "Lib/String.h"

BUFF_NAMESPACE_BEGIN

TEST_CASE("FilePath conversion") {
    const String accents =
        u8"filename-with-utf-"
        u8"\x68\xC3\xA1\xC4\x8D\x65\x6B\x20\xE3\x83\xBE\x28\xE2\x89\xA7\xE2\x96\xBD\xE2\x89\xA6\x2A\x29\x6F";

    CHECK(FilePath(accents).getNative() == accents);
    CHECK(FilePath(accents).getGeneric() == accents);

    CHECK("a/b\\c"_File.getNative() == "a\\b\\c");
    CHECK("a/b\\c"_File.getGeneric() == "a/b/c");
}

// ===========================================================================================================
// FilePath
// ===========================================================================================================

TEST_CASE("FilePath::constructor") {
    CHECK_ASSERT(FilePath("/var/tmp/example.txt/"));
    CHECK_ASSERT(FilePath("var/tmp/example.txt/"));
    CHECK_ASSERT(FilePath("/var/tmp/example.txt"));
    CHECK(FilePath().getNative() == "");
}

TEST_CASE("FilePath::getDirectory") {
    CHECK(!FilePath().getDirectory());
    CHECK(!FilePath("aaaa").getDirectory());
    CHECK(!FilePath("aaaa.exe").getDirectory());
    CHECK(FilePath("C:/var").getDirectory() == "C:/"_Dir);
    CHECK(FilePath("C:/var.exe").getDirectory() == "C:/"_Dir);
    CHECK(FilePath("C:/var/tmp/foo").getDirectory() == "C:/var/tmp/"_Dir);
    CHECK(FilePath("C:/var/tmp/foo.txt").getDirectory() == "C:/var/tmp/"_Dir);
    CHECK(FilePath("var/tmp/example").getDirectory() == "var/tmp/"_Dir);
    CHECK(FilePath("var/tmp/example.txt").getDirectory() == "var/tmp/"_Dir);
}

TEST_CASE("FilePath::getFilenameWithExtension") {
    CHECK(FilePath("C:/var/tmp/example.txt").getFilenameWithExtension() == "example.txt");
    CHECK(FilePath("var/tmp.a.b").getFilenameWithExtension() == "tmp.a.b");
    CHECK(FilePath("var.a.b/tmp.a.b").getFilenameWithExtension() == "tmp.a.b");
    CHECK(FilePath("aaaa").getFilenameWithExtension() == "aaaa");
    CHECK(FilePath("foo/.aaaa").getFilenameWithExtension() == ".aaaa");
    CHECK(FilePath("C:/aaa/aaaa").getFilenameWithExtension() == "aaaa");
    CHECK(FilePath("C:/aaa.bbb/aaaa").getFilenameWithExtension() == "aaaa");
    CHECK(FilePath().getFilenameWithExtension() == "");
}

TEST_CASE("FilePath::getFilenameWithoutExtension") {
    CHECK(FilePath("C:/var/tmp/example.txt").getFilenameWithoutExtension() == "example");
    CHECK(FilePath("var/tmp.a.b").getFilenameWithoutExtension() == "tmp.a");
    CHECK(FilePath("var.a.b/tmp.a.b").getFilenameWithoutExtension() == "tmp.a");
    CHECK(FilePath("aaaa").getFilenameWithoutExtension() == "aaaa");
    CHECK(FilePath("foo/.aaaa").getFilenameWithoutExtension() == "");
    CHECK(FilePath("C:/aaa/aaaa").getFilenameWithoutExtension() == "aaaa");
    CHECK(FilePath("C:/aaa.bbb/aaaa").getFilenameWithoutExtension() == "aaaa");
    CHECK(FilePath().getFilenameWithoutExtension() == "");
}

TEST_CASE("FilePath::getExtension") {
    CHECK(FilePath("C:/var/tmp/example.txt").getExtension() == "txt");
    CHECK(FilePath("var/tmp.a.b").getExtension() == "b");
    CHECK(FilePath("var.a.b/tmp.a.b").getExtension() == "b");
    CHECK(!FilePath("aaaa").getExtension());
    CHECK(!FilePath("C:/aaa/aaaa").getExtension());
    CHECK(!FilePath("C:/aaa.bbb/aaaa").getExtension());
    CHECK(!FilePath().getExtension());
}

// ===========================================================================================================
// DirectoryPath
// ===========================================================================================================

TEST_CASE("DirectoryPath::constructor") {
    CHECK_ASSERT("/var/tmp/example.txt/"_Dir);
    CHECK_ASSERT("/var/tmp/example.txt"_Dir);
    CHECK(DirectoryPath().getNative() == "");
}

TEST_CASE("DirectoryPath::getParentFolder") {
    CHECK(!DirectoryPath().getParentFolder());
    CHECK(!"aaaa"_Dir.getParentFolder());
    CHECK(!"aaaa.exe"_Dir.getParentFolder());
    CHECK("C:/var"_Dir.getParentFolder() == DirectoryPath("C:/"));
    CHECK("C:/var.exe"_Dir.getParentFolder() == DirectoryPath("C:/"));
    CHECK("C:/var/tmp/foo"_Dir.getParentFolder() == DirectoryPath("C:/var/tmp/"));
    CHECK("C:/var/tmp/foo.txt"_Dir.getParentFolder() == DirectoryPath("C:/var/tmp/"));
    CHECK("var/tmp/example"_Dir.getParentFolder() == DirectoryPath("var/tmp/"));
    CHECK("var/tmp/example.txt"_Dir.getParentFolder() == DirectoryPath("var/tmp/"));

    CHECK(!"aaaa/"_Dir.getParentFolder());
    CHECK(!"aaaa.exe/"_Dir.getParentFolder());
    CHECK("C:/var/"_Dir.getParentFolder() == DirectoryPath("C:/"));
    CHECK("C:/var.exe/"_Dir.getParentFolder() == DirectoryPath("C:/"));
    CHECK("C:/var/tmp/foo/"_Dir.getParentFolder() == DirectoryPath("C:/var/tmp/"));
    CHECK("C:/var/tmp/foo.txt/"_Dir.getParentFolder() == DirectoryPath("C:/var/tmp/"));
    CHECK("var/tmp/example/"_Dir.getParentFolder() == DirectoryPath("var/tmp/"));
    CHECK("var/tmp/example.txt/"_Dir.getParentFolder() == DirectoryPath("var/tmp/"));
}

TEST_CASE("DirectoryPath::getLastSegment") {
    CHECK(DirectoryPath().getLastSegment() == "");
    CHECK("aaaa"_Dir.getLastSegment() == "aaaa");
    CHECK("aaaa.exe"_Dir.getLastSegment() == "aaaa.exe");
    CHECK("C:/var"_Dir.getLastSegment() == "var");
    CHECK("C:/var.exe"_Dir.getLastSegment() == "var.exe");
    CHECK("C:/var/tmp/foo"_Dir.getLastSegment() == "foo");
    CHECK("C:/var/tmp/foo.txt"_Dir.getLastSegment() == "foo.txt");
    CHECK("var/tmp/example"_Dir.getLastSegment() == "example");
    CHECK("var/tmp/example.txt"_Dir.getLastSegment() == "example.txt");

    CHECK("aaaa/"_Dir.getLastSegment() == "aaaa");
    CHECK("aaaa.exe/"_Dir.getLastSegment() == "aaaa.exe");
    CHECK("C:/var/"_Dir.getLastSegment() == "var");
    CHECK("C:/var.exe/"_Dir.getLastSegment() == "var.exe");
    CHECK("C:/var/tmp/foo/"_Dir.getLastSegment() == "foo");
    CHECK("C:/var/tmp/foo.txt/"_Dir.getLastSegment() == "foo.txt");
    CHECK("var/tmp/example/"_Dir.getLastSegment() == "example");
    CHECK("var/tmp/example.txt/"_Dir.getLastSegment() == "example.txt");
}

TEST_CASE("DirectoryPath::operator/") {
    CHECK("a"_Dir / FilePath("b") == FilePath("a/b"));
    CHECK("a/b/c"_Dir / FilePath("b/c/d") == FilePath("a/b/c/b/c/d"));
    CHECK("a/b/c/"_Dir / FilePath("b/c/d") == FilePath("a/b/c/b/c/d"));
    CHECK("a/b/c/"_Dir / FilePath("b/c/d") == FilePath("a/b/c/b/c/d"));
}

BUFF_NAMESPACE_END
