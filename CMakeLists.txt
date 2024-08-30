cmake_minimum_required(VERSION 3.27)

set(COMPILER           "compiler" CACHE STRING "Choose a compiler: cl, clang, emscripten")
set(WARNINGS_AS_ERRORS 0          CACHE BOOL   "Enables/Disables warnings as errors")
if(NOT COMPILER MATCHES "^(cl|clang|emscripten)$")
    message(FATAL_ERROR "Invalid option for COMPILER. Choose from: cl, clang, emscripten.")
endif()

if(COMPILER STREQUAL clang)
    set(CMAKE_GENERATOR_TOOLSET "ClangCL")
endif()
if(COMPILER STREQUAL emscripten)
    set(EMSCRIPTEN           1)
    set(WINDOWS              0)
    set(VCPKG_TARGET_TRIPLET wasm32-emscripten)
    project(BUFF_${COMPILER})
else()
    set(EMSCRIPTEN           0)
    set(WINDOWS              1)
    set(VCPKG_TARGET_TRIPLET x64-windows-static)
    # The .sln project file is later modified in python and saved to the final filename without the suffix
    project(BUFF_${COMPILER}_original)
endif()

set(CMAKE_CXX_STANDARD          23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CONFIGURATION_TYPES   "Debug;Release" CACHE STRING "" FORCE)
set(CMAKE_SUPPRESS_REGENERATION true)  # Suppress generation of ZERO_CHECK

set(TMP_DIR              "${CMAKE_SOURCE_DIR}/../_tmp")
set(DEPENDENCIES_DIR     "${TMP_DIR}/vcpkg")
set(VCPKG_ROOT           "${DEPENDENCIES_DIR}")
include("${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")  # The recommended set(CMAKE_TOOLCHAIN_FILE ...) is not working...

include("Buff/Scripts/Warnings.cmake")
include("Buff/Scripts/Assets.cmake")
include("Buff/Scripts/Functions.cmake")

set(DEFAULT_GLOB_FILTER
    "*.cpp"
    "*.h"
    "*.rc"
    "CmakeLists.txt")

add_subdirectory(Buff)

# Add all other potentially present folders
file(GLOB folders "${CMAKE_SOURCE_DIR}/*")
foreach(entry ${folders})
    if(IS_DIRECTORY "${entry}")
        get_filename_component(subfolder "${entry}" NAME)
        if((NOT subfolder STREQUAL "Buff") AND (NOT subfolder STREQUAL ".git"))
            message(">> Processing subfolder '${subfolder}'")
            add_subdirectory("${subfolder}")
        endif()
    endif()
endforeach()
