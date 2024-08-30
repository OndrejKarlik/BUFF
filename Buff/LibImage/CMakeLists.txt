setupLib(LibImage)
    find_package(SDL2 CONFIG REQUIRED)
    if(NOT EMSCRIPTEN)
        find_package(SDL2_Image CONFIG REQUIRED)
        target_link_libraries($ENV{CURRENT} INTERFACE
            LibSdl
        )
        target_link_libraries($ENV{CURRENT} INTERFACE
              $<IF:$<TARGET_EXISTS:SDL2_image::SDL2_image>,SDL2_image::SDL2_image,SDL2_image::SDL2_image-static>
        )
    endif()

