# TODO: consider renaming this utility lib to 'platform_warnings' or 'strict_warnings' or something similar to make it more descriptive
add_library(warnings INTERFACE)
target_compile_options(warnings
    INTERFACE
        -Wall
        -Wextra
        -Wshadow
        # warn for potential performance problem casts
        -Wcast-align
        -Wunused
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wnull-dereference
        # warn if float is implicit promoted to double
        -Wdouble-promotion
        # warn on security issues around functions that format output (ie printf)
        -Wformat=2
        -Wtype-limits

        # TODO: i am suspicious of disabling warnings globally, even tho it was recommended, so im disabling these...
        #
        # -Wno-unused-parameter
        # -Wno-error=unused-parameter
        # -Wno-unused-private-field
        # -Wno-error=unused-private-field
        # disable abi warnings as we always compile the whole programm and do not link precompiled libraries
        # -Wno-psabi

        # warn the user if a class with virtual functions has a non-virtual destructor.
        "$<$<COMPILE_LANGUAGE:CXX>:-Wnon-virtual-dtor>"
        "$<$<COMPILE_LANGUAGE:CXX>:-Wold-style-cast>"
        # warn if you overload (not override) a virtual function
        "$<$<COMPILE_LANGUAGE:CXX>:-Woverloaded-virtual>"

        "$<$<BOOL:${ENABLE_WARNINGS_AS_ERRORS}>:-Werror>"
)
