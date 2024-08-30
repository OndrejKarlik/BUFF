setupLib(LibSdl)
    find_package(SDL2 CONFIG REQUIRED)
    if(NOT EMSCRIPTEN)
        find_package(SDL2_TTF CONFIG REQUIRED)
        find_package(Freetype CONFIG REQUIRED)
        target_link_libraries($ENV{CURRENT} INTERFACE
            Freetype::Freetype
            # The weird syntax here is provided by vcpkg
            # If this is enabled, we get duplicate wmain and main: $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
            $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
            $<IF:$<TARGET_EXISTS:SDL2_ttf::SDL2_ttf>,SDL2_ttf::SDL2_ttf,SDL2_ttf::SDL2_ttf-static>
       )
    target_link_options($ENV{CURRENT} INTERFACE /NODEFAULTLIB:libcmt$<IF:$<CONFIG:Debug>,d,>.lib)
    endif()

