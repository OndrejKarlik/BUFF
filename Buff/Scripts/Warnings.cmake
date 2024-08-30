function(setupWarnings)
    # Emscripten vs. the rest:
    if (EMSCRIPTEN)
        target_compile_options(${name} PRIVATE
            -Wno-shorten-64-to-32 # Because emscripten compiles x86
            -Wno-deprecated-builtins # In robin_hood.h
            -Wno-disabled-macro-expansion # emscripten does: #define stderr (strerr)
            -Wno-old-style-cast # Expanded from macros from Imgui
            # Warnings that are turned on in emscripten but not in normal clang. TODO: enable in normal clang?
            -Wno-switch-default
            -Wno-padded
            -Wno-weak-vtables
            -Wno-missing-field-initializers
        )
    else()
        target_compile_options(${name} PRIVATE
            /Wall
            /external:anglebrackets
            /external:W0
            $<$<BOOL:${WARNINGS_AS_ERRORS}>:/WX>
        )
    endif()

    # CL vs the rest:

    if(COMPILER STREQUAL "cl")
        target_compile_options(${name} PRIVATE
            /wd4365 # 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
            /wd4820 # '4' bytes padding added after data member 'AssertArguments::lineNumber'
            /wd4514 # 'Rgb8Bit::Rgb8Bit': unreferenced inline function has been removed
            /wd5045 # Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
            /wd4061 # enumerator 'TokenType::UNKNOWN_CHAR' in switch of enum 'TokenType' is not explicitly
                    # handled by a case label
            /wd4625 # copy constructor was implicitly defined as deleted
            /wd5026 # move constructor was implicitly defined as deleted
            /wd4626 # assignment operator was implicitly defined as deleted
            /wd5027 # move assignment operator was implicitly defined as deleted
            /wd4866 # compiler may not enforce left-to-right evaluation order for call to X
            /wd5245 # 'doctest::`anonymous-namespace'::XmlWriter::ScopedElement::operator =': unreferenced 
                    # function with internal linkage has been removed"
            /wd4711 # function X selected for automatic inline expansion
            /wd4710 # '__cdecl Token::Comment::~Comment(void) __ptr64': function not inlined"
            /wd5264 # 'const' variable is not used - fired for global variables in general headers that are
                    # not used in the specific unit
            /wd4191 # 'reinterpret_cast': unsafe conversion from 'FARPROC' to 'x'
            /wd4868 # compiler may not enforce left-to-right evaluation order in braced initializer list
            /wd4623 # default constructor was implicitly defined as deleted - silencing it would mean deleting
                    # default constructor, but that then prohibits default intialization from initializer list

            # Warnings as errors
            /we4715 # not all control paths return a value - leads to a crash when overlooked
            /we4129 # unrecognized character escape sequence
        )
    else()
        target_compile_options(${name} PRIVATE
            -Weverything

            -Wno-c++98-compat
            -Wno-c++98-compat-pedantic
            -Wno-c++20-compat
            -Wno-c++98-c++11-c++14-compat
            -Wno-exit-time-destructors
            -Wno-global-constructors
            -Wno-ctad-maybe-unsupported
            -Wno-shadow-field-in-constructor
            -Wno-gnu-zero-variadic-macro-arguments
            -Wno-ambiguous-reversed-operator
            -Wno-sign-compare
            -Wno-switch-enum
            -Wno-covered-switch-default
            -Wno-shadow-uncaptured-local
            -Wno-sign-conversion
            -Wno-header-hygiene
            -Wno-unsafe-buffer-usage
            -Wno-float-equal
            -Wno-cast-function-type
            -Wno-c++20-extensions
            -Wno-trigraphs
            -Wno-date-time
            -Wno-undefined-reinterpret-cast
            -Wno-zero-as-null-pointer-constant # Bugged for <=> operator in clang 16
        )
        set_target_properties(${name} PROPERTIES VS_GLOBAL_ExternalIncludePath
            "${DEPENDENCIES_DIR}/installed/${VCPKG_TARGET_TRIPLET}/include"
        )
    endif()
endfunction()
