setupExecutable(EmscriptenHelloWorld)
    target_link_libraries($ENV{CURRENT} PRIVATE
        Lib
        LibSdl
        LibImage
    )
    setGroup("Sandboxes")
    file(CREATE_LINK "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$ENV{RUN_DIR}/assets" SYMBOLIC)
