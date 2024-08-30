setupLib(LibWindows)
    target_link_libraries($ENV{CURRENT} INTERFACE
        Rpcrt4.lib
        Wininet.lib
        # Needed for new style of File Open/Save dialogs
        Shlwapi.lib
        Comctl32.lib
        Propsys.lib
    )
    target_compile_definitions($ENV{CURRENT} PUBLIC
        STRICT
        STRICT_TYPED_ITEMIDS
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )

setupTest(LibWindows.Test)
    target_link_libraries($ENV{CURRENT} PRIVATE
        Lib
        LibWindows
    )