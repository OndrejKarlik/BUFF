setupExecutable(CodeFormatter)
    target_link_libraries($ENV{CURRENT} PRIVATE
        Lib
        $<${WINDOWS}:LibWindows>
    )
    setDebuggerArgs("${CMAKE_SOURCE_DIR}/.tools/clang-format/clang-format.exe ${TMP_DIR}/.clang-format-cache-debugging ${CMAKE_SOURCE_DIR}")
    setGroup("Tools")
