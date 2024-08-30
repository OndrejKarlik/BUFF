set(SUBMODULE "Buff")

add_subdirectory(Lib)
add_subdirectory(LibSdl)
add_subdirectory(LibImgui)
add_subdirectory(LibImage)
if(NOT EMSCRIPTEN)
    add_subdirectory(LibWindows)
    add_subdirectory(LibUltralight)
endif()

# Sandboxes
add_subdirectory(Sandbox)
add_subdirectory(ImguiHelloWorld)
add_subdirectory(EmscriptenHelloWorld)

if(NOT EMSCRIPTEN)
    add_subdirectory(CodeFormatter)
    add_subdirectory(Benchmark)
endif()

if(NOT EMSCRIPTEN)
    include_external_msproject(TsDevServer "${CMAKE_CURRENT_SOURCE_DIR}/TsDevServer/TsDevServer.esproj" PLATFORM "Any CPU" TYPE "54A90642-561A-4BB1-A94E-469ADEE60C69")
    include_external_msproject(_Scripts    "${CMAKE_CURRENT_SOURCE_DIR}/Scripts/Scripts.esproj"         PLATFORM "Any CPU" TYPE "54A90642-561A-4BB1-A94E-469ADEE60C69")
    set_target_properties("TsDevServer" PROPERTIES FOLDER "Web")
endif()
