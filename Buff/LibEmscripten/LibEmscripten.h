#pragma once
#include "Lib/Assert.h"
#include "Lib/Path.h"
#include <emscripten.h>

BUFF_NAMESPACE_BEGIN

inline bool isDebuggerPresent() {
    return false;
}

inline void debuggingAssertHandler(const AssertArguments& assert) {
    getDefaultAssertHandler()(assert);
}

inline FilePath getTemporaryFilename() {
    static int64 index = 0;
    return FilePath("../tmp/emscripten_tmp/" + toStr(index++) + ".tmp");
}

enum class ConsoleColor { BLUE, RED, GREEN, YELLOW, CYAN, MAGENTA };

inline String makeStringLiteral(String str) {
    str.replaceAll("\\", "\\\\");
    str.replaceAll("\"", "\\\""); // TODO: Is this enough?
    return "\"" + str + "\"";
}

inline void printStdoutColored(const StringView message, ConsoleColor color) {
    const String colorName = [&]() {
        switch (color) {
        case ConsoleColor::BLUE:
            return "blue";
        case ConsoleColor::RED:
            return "red";
        case ConsoleColor::GREEN:
            return "green";
        case ConsoleColor::YELLOW:
            return "yellow";
        case ConsoleColor::CYAN:
            return "cyan";
        case ConsoleColor::MAGENTA:
            return "magenta";
        }
    }();
    emscripten_run_script(
        ("console.log(" + makeStringLiteral("%c" + message) + ", ' color : " + colorName + "');")
            .asCString());
}

inline void setConsoleTitle(const StringView title) {
    emscripten_run_script(("document.title = " + makeStringLiteral(title) + ";").asCString());
}

BUFF_NAMESPACE_END
