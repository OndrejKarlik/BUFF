function(getUltralightDir output)
    set(NEW_UL 2)
    if(NEW_UL EQUAL 2)
        set(${output} "${TMP_DIR}/ultralight-sdk-latest-win-x64" PARENT_SCOPE)
    elseif(NEW_UL EQUAL 1)
        set(${output} "${TMP_DIR}/ultralight-sdk-1.4.0b-win-x64-debug" PARENT_SCOPE)
    else()
        set(${output} "${TMP_DIR}/ultralight/SDK" PARENT_SCOPE)
    endif()
endFunction()

function(addUltralight)
    # message("DEBUG inside addUltralight: ${LIB_ULTRALIGHT_SOURCES}")
    get_target_property(externalIncludePath LibUltralight VS_GLOBAL_ExternalIncludePath)
    set_target_properties($ENV{CURRENT} PROPERTIES VS_GLOBAL_ExternalIncludePath "${externalIncludePath}")
    target_link_libraries($ENV{CURRENT} PRIVATE
        LibUltralight
    )

    getUltralightDir(ultralightSdkDir)
    if(NOT EXISTS "${ultralightSdkDir}")
        message(FATAL_ERROR "Ultralight SDK not found in ${ultralightSdkDir}")
    endif()
    set(assetsSubFolder        "$ENV{RUN_DIR}/assets")
    set(ultralightOutputFolder "${CMAKE_BINARY_DIR}/Buff/LibUltralight/web/Buff/LibUltralight/assets")

    # We need to make these folders, otherwise the next commands fail
    file(MAKE_DIRECTORY "${assetsSubFolder}")
    file(MAKE_DIRECTORY "${ultralightOutputFolder}")

    file(CREATE_LINK "${ultralightSdkDir}/resources" "${assetsSubFolder}/resources"     SYMBOLIC)
    file(CREATE_LINK "${ultralightSdkDir}/inspector" "${assetsSubFolder}/inspector"     SYMBOLIC)
    file(CREATE_LINK "${ultralightOutputFolder}"     "${assetsSubFolder}/LibUltralight" SYMBOLIC) 

    file(GLOB ULTRALIGHT_BINARIES CONFIGURE_DEPENDS "${ultralightSdkDir}/bin/*.*")
    foreach(binary ${ULTRALIGHT_BINARIES})
        get_filename_component(name "${binary}" NAME)
        file(CREATE_LINK "${binary}" "$ENV{RUN_DIR}/${name}" SYMBOLIC) 
    endforeach()
endfunction()

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${DEFAULT_GLOB_FILTER})
file(GLOB_RECURSE assets CONFIGURE_DEPENDS "assets/*")
list(APPEND SOURCES ${assets})

setupLib(LibUltralight "${SOURCES}")
    getUltralightDir(ultralightSdkDir)

    set(ultralightInclude ${ultralightSdkDir}/include)
    target_include_directories($ENV{CURRENT} PUBLIC
        ${ultralightInclude}
    )
    if(COMPILER STREQUAL "clang")
        get_target_property(previousExternalIncludePath $ENV{CURRENT} VS_GLOBAL_ExternalIncludePath)
        set_target_properties($ENV{CURRENT} PROPERTIES VS_GLOBAL_ExternalIncludePath
            "${previousExternalIncludePath};${ultralightInclude}"
        )
    endif()
    target_link_directories($ENV{CURRENT} INTERFACE
        ${ultralightSdkDir}/lib
    )
    target_link_libraries($ENV{CURRENT} INTERFACE
        AppCore.lib
        Ultralight.lib
        UltralightCore.lib
        WebCore.lib
    )
