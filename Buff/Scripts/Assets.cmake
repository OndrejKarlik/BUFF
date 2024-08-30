function(setupAssets)

    # Get list of all files that the current project TS files depend on. This needs to be done better in the future
    file(GLOB_RECURSE ALL_TS_FILES CONFIGURE_DEPENDS "*.ts") # Everything in the current project
    set(libUltralightAssets "${CMAKE_SOURCE_DIR}/Buff/LibUltralight/assets")
    file(GLOB_RECURSE ALL_TS_FILES_TMP CONFIGURE_DEPENDS "${libUltralightAssets}/*.ts")
    if(NOT ALL_TS_FILES_TMP)
        message(FATAL_ERROR "Error: Not TS files in LibUltralight?")
    endif()
    list(APPEND ALL_TS_FILES ${ALL_TS_FILES_TMP}) # Append everything from LibUltralight/
        
    set(ASSET_SOURCES "${SOURCES}")
    list(FILTER ASSET_SOURCES INCLUDE REGEX "/assets/")
    list(FILTER ASSET_SOURCES EXCLUDE REGEX "\\.d\\.ts$") # .d.ts files are neither compiled nor copied

    # Handle assets compilation/copying if present. Note that we need this peculiar folder structure to deal with TypeScript modules resolving, 
    # which is quite inflexible - it requires the same file hierarchy on input and output, and it always compiles dependencies and produces 
    # all outputs when compiling a single file
    foreach(file ${ASSET_SOURCES})
        # message("    ${file}")
        # File starts as absolute path.
        # Get path of input asset relative to root repository folder
        # E.g.: file = Project/assets/Asset.ext
        cmake_path(RELATIVE_PATH file BASE_DIRECTORY "${CMAKE_SOURCE_DIR}" OUTPUT_VARIABLE relativePath)
        # E.g. outFolder = ../build/Project/web
        set(outFolder "${CMAKE_CURRENT_BINARY_DIR}/web")
        # E.g. outFile = ../build/Project/web/Project/assets/Asset.ext
        # outFile might get modified later to replace its extension (e.g. ts -> js, less -> css)
        set(outFile "${outFolder}/${relativePath}")

        # Note that the output path is going to be LONG and contain repeated parts: 
        # build_compiler/submodule/project/web/submodule/project/assets/Asset.ext
        # This is because typescript always compiles also imported modules and replicates input folder structure in 
        # the output. We could in theory make it shallower by setting the root to build_dir, but that would 
        # mean building a dependent project would rebuild assets in dependencies, potentially leading to race
        # conditions

        string(REGEX MATCH ".ts$"   isTs   "${file}")
        string(REGEX MATCH ".less$" isLess "${file}")

        if(isTs)
            # Create a unique filename for unique tsconfig.json per each compiled TS file
            # Note that we are doing this because some options cannot be passed on cmdline
            cmake_path(RELATIVE_PATH file BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" OUTPUT_VARIABLE relativeToProject)
            string(REPLACE "/" "_" relativeToProject "${relativeToProject}")
            string(REPLACE "." "_" relativeToProject "${relativeToProject}")
            set(tsConfigPath "${CMAKE_CURRENT_BINARY_DIR}/${relativeToProject}_tsconfig.json")
            # message(${tsConfigPath})

            # message("TS: ${file}")
            cmake_path(REMOVE_EXTENSION outFile LAST_ONLY)
            set(outFile "${outFile}.js")
            set(TS_TARGET_CHROME 0)
            if(TS_TARGET_CHROME EQUAL 1)
                set(TS_COMPILE_TARGET "esnext")
            else()
                # Targetting es2019 because it downgrades the "?." operator not yet present in Ultralight WebKit
                set(TS_COMPILE_TARGET "es2019")
            endif()
            add_custom_command(
                OUTPUT ${outFile}
                COMMAND tsc --project "${tsConfigPath}"
                MAIN_DEPENDENCY ${file}
                DEPENDS ${ALL_TS_FILES} # TODO: currently we depend everything on everything
                COMMENT "[TS] Compiling ${relativePath}" # ${file} -> ${outFile}"
            )
            set(tsConfig 
                " {
                      \"compilerOptions\" : {
                          \"outDir\"        : \"${outFolder}\",
                          \"target\"        : \"${TS_COMPILE_TARGET}\", /* JavaScript target (lower values mean some new features get replaced for compatibility) */
                          \"rootDir\"       : \"${CMAKE_SOURCE_DIR}\",  
                          \"noEmitOnError\" : true,                     /* Do not emit .js output if there is an error. */
                          \"strict\"        : true,                     /* enable more checks */
                          \"module\"        : \"esnext\",               /* Needed for import.meta */

                          \"inlineSourceMap\" : true,  /* These 2 options allow us to view and debug original source code in a browser */
                          \"inlineSources\"   : true,

                          \"paths\" : { /* Allows us to use much shorter path to access LibUltralight files from other projects  */
                              \"LibUltralight/*\" : [ \"${libUltralightAssets}/*\" ]
                          }
                      },
                      \"files\": [ \"${file}\" ]
                  }"
            )
            # --incremental                                       # Speed up compilation by caching information about the project
            # --tsBuildInfoFile  "${TMP_DIR}/tsBuildInfoFile.txt" # Where to store the --incremental cache
            # --strictNullChecks false 
            # --moduleResolution bundler
            file(WRITE "${tsConfigPath}" "${tsConfig}")

        elseif(isLess)
            # message("Less: ${file}")
            cmake_path(REMOVE_EXTENSION outFile LAST_ONLY)
            set(outFile "${outFile}.css")
            add_custom_command(
                OUTPUT  ${outFile}
                COMMAND lessc "${file}" "${outFile}"
                MAIN_DEPENDENCY ${file}
                DEPENDS ${file}
                COMMENT "[LESS] Compiling ${relativePath}" # ${file} -> ${outFile}"
            )
        else()
            # message("Asset: ${file}")
            add_custom_command(
                OUTPUT  ${outFile}
                COMMAND ${CMAKE_COMMAND} -E copy "${file}" "${outFile}"
                MAIN_DEPENDENCY ${file}
                DEPENDS ${file}
                COMMENT "[asset] Copying ${relativePath}" # ${file} -> ${outFile}"
            )
        endif()

        # Link the compiled asset to rundir
        cmake_path(RELATIVE_PATH file BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" OUTPUT_VARIABLE relativeFile)
        cmake_path(REMOVE_EXTENSION relativeFile LAST_ONLY)
        get_filename_component(extension "${outFile}" LAST_EXT)

        # message("${outFile} $ENV{RUN_DIR}/assets/$ENV{CURRENT}/${relativeFile}${extension}")
     
        get_filename_component(linkFolder "${relativeFile}" DIRECTORY)
        file(MAKE_DIRECTORY "$ENV{RUN_DIR}/${linkFolder}")
        file(CREATE_LINK "${outFile}" "$ENV{RUN_DIR}/${relativeFile}${extension}" SYMBOLIC)

    endforeach()

endfunction()