setupExecutable(Benchmark)
    target_link_libraries($ENV{CURRENT} PRIVATE
        Lib
        $<${WINDOWS}:LibWindows>
    )
    setGroup("Tools")
    setDebuggerArgs("timeout 2 && echo \"Test 1 2 3\" exit 5")
