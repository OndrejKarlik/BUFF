setupExecutable(ImguiHelloWorld)
    target_link_libraries($ENV{CURRENT} PRIVATE
        Lib
        $<${WINDOWS}:LibWindows>
    )
    addImgui()
    file(CREATE_LINK "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$ENV{RUN_DIR}/assets/$ENV{CURRENT}" SYMBOLIC)
    setGroup("Sandboxes")
