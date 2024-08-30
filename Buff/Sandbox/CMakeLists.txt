setupExecutable(Sandbox)
    target_link_libraries($ENV{CURRENT} PRIVATE
        Lib
        $<${WINDOWS}:LibWindows>
    )
    setGroup("Sandboxes")
