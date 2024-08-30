
function(setDebuggerArgs args)
    set_target_properties($ENV{CURRENT} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS ${args})
endfunction()

function(setGroup group)
    set_target_properties($ENV{CURRENT} PROPERTIES FOLDER ${group})
endfunction()

function(setGuiExecutable)
    set_target_properties($ENV{CURRENT} PROPERTIES WIN32_EXECUTABLE TRUE)
endfunction()

function(setupGeneral name)
    set(ENV{CURRENT}    "${name}")
    set(ENV{RUN_DIR}    "${TMP_DIR}/rundir/${SUBMODULE}/${name}")
    file(MAKE_DIRECTORY "$ENV{RUN_DIR}")

    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES}) # Sets virtual folders in MSVS project
    target_include_directories(${name} PRIVATE
        "${CMAKE_SOURCE_DIR}/${SUBMODULE}"
        "${CMAKE_SOURCE_DIR}/Buff" # Buff is visible from everywhere
        "${DEPENDENCIES_DIR}/installed/${VCPKG_TARGET_TRIPLET}/include" # TODO: Shouldn't this be handled by VCPKG automatically?!
    )
    #message("_________________DEBUG: $ENV{CURRENT}")

    set_target_properties(${name} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY $ENV{RUN_DIR})
    target_compile_definitions(${name} PRIVATE
        _UNICODE
        UNICODE
        STRICT=1
        WIN32_LEAN_AND_MEAN=1
        NOMINMAX=1
        BUFF_DEBUG=$<IF:$<CONFIG:Debug>,1,0>
        BUFF_RELEASE=$<IF:$<CONFIG:Debug>,0,1>
        $<IF:$<CONFIG:Debug>,DEBUG _DEBUG,NDEBUG>
    )
    if (EMSCRIPTEN)
        target_compile_options(${name} PRIVATE
            -sUSE_SDL=2 -sUSE_SDL_IMAGE=2 -sUSE_SDL_TTF=2 -sWASM=1 -sALLOW_MEMORY_GROWTH=1 -sMAX_WEBGL_VERSION=3
        )
        target_link_options(${name} PRIVATE
            -sUSE_SDL=2 -sUSE_SDL_IMAGE=2 -sUSE_SDL_TTF=2 -o ${name}.html
            # Causes internal compiler error... but without it PNG loading does not work...
            #-sSDL2_IMAGE_FORMATS=["png"]
        )
    else() # cl + clang
        target_compile_options(${name} PRIVATE
            # General
            /MP # Multithreaded compilation
            /Zi # Debug information: Program Database
            /GF # Enable string pooling
            /FC # Full path to source code in diagnostics
            /EHsc # Enable exceptions (now turned off by default in clang for some reason...)
            /utf-8 # Set compiler source code and runtime code page to UTF8
            /Zc:preprocessor # standard-conforming preprocessor (using __VA_OPT__)
            /arch:AVX2
            $<IF:$<CONFIG:Debug>, , /Oi /GS-> # Release: Enable intrinsic functions, disable security checks
        )
        target_link_options(${name} PRIVATE
            /DEBUG # Generate PDBs also in Release
        )
        if(COMPILER STREQUAL "clang")
            target_compile_options(${name} PRIVATE
                -showFilenames
            )
        endif()
    endif()
        
    setupWarnings()
    setupAssets() 
endfunction()

# Second argument is optional list of files
function(setupLib name)
    message("Creating library: ${name}")
    if(${ARGC} GREATER 1)
        set(SOURCES ${ARGV1})
        # message("#### Using already provided sources: ${ARGV1}")
    else()
        file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${DEFAULT_GLOB_FILTER})
        # Remove files ending with .Test.cpp or .Test.h
        list(FILTER SOURCES EXCLUDE REGEX ".*\\.(Test\\.cpp|Test\\.h)$")
    endif()
    add_library(${name} STATIC ${SOURCES})
    setupGeneral(${name})
endfunction()

function(setupDll name)
    if(${ARGC} GREATER 1)
        error("Explicit list of files not supported yet")
    endif()
    message("Creating DLL: ${name}")
    file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${DEFAULT_GLOB_FILTER})
    # Remove files ending with .Test.cpp or .Test.h
    list(FILTER SOURCES EXCLUDE REGEX ".*\\.(Test\\.cpp|Test\\.h)$")
    if(EMSCRIPTEN)
        add_library(${name} STATIC ${SOURCES})
    else()
        add_library(${name} SHARED ${SOURCES})
    endif()
    setupGeneral(${name})
endfunction()

function(setupTest name)
    if(${ARGC} GREATER 1)
        error("Explicit list of files not supported yet")
    endif()
    message("Creating unit tests: ${name}")
    file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${DEFAULT_GLOB_FILTER})
    list(FILTER SOURCES INCLUDE REGEX ".*\\.(Test\\.cpp|Test\\.h)$")
    add_executable(${name} ${SOURCES})
    setupGeneral(${name})
endfunction()

function(setupExecutable name)
    message("Creating executable: ${name}")
    if(${ARGC} GREATER 1)
        set(SOURCES ${ARGV1})
        # message("#### Using already provided sources: ${ARGV1}")
    else()
        file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${DEFAULT_GLOB_FILTER})
        # Remove files ending with .Test.cpp or .Test.h
        list(FILTER SOURCES EXCLUDE REGEX ".*\\.(Test\\.cpp|Test\\.h)$")
    endif()
    add_executable(${name} ${SOURCES})
    setupGeneral(${name})
endfunction()
