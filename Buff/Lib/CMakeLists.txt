setupLib(Lib)
    find_package(utf8cpp CONFIG REQUIRED)

setupTest(Lib.Test)
    target_link_libraries($ENV{CURRENT} PRIVATE
        Lib
        $<${WINDOWS}:LibWindows>
    )
