function(addImgui)
    target_link_libraries($ENV{CURRENT} PRIVATE
        LibSdl
        LibImgui
    )
    set(assetsDir "$ENV{RUN_DIR}/assets")
    # We need to make this folder, otherwise the next command fails
    file(MAKE_DIRECTORY "${assetsDir}")
    set(assetsToLink "${CMAKE_SOURCE_DIR}/Buff/LibImgui/assets")
    if(NOT EXISTS "${assetsToLink}") 
        message(FATAL_ERROR "ERROR: Imgui assets folder not found, did it move?")
    endif()
    file(CREATE_LINK "${assetsToLink}" "${assetsDir}/LibImgui" SYMBOLIC) 
endfunction()

setupLib(LibImgui)
    target_compile_definitions($ENV{CURRENT} PUBLIC
        # The library in dependencies is probably compiled without this flag. Enabling it causes a sizeof mismatch
        # IMGUI_DISABLE_OBSOLETE_FUNCTIONS
        IMGUI_USER_CONFIG="LibImgui/ImguiConfigInjection.h"
    )
    find_package(imgui CONFIG REQUIRED)
    target_link_libraries($ENV{CURRENT} INTERFACE
        LibSdl
        LibImage
        imgui::imgui
        $<${WINDOWS}:OpenGL32.lib>
    )
