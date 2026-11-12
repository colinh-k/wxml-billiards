cmake_minimum_required(VERSION 3.31)
project(simulator
    LANGUAGES
        CXX
)

option(ENABLE_TESTING "build and run the test targets for each build type" TRUE)
option(ENABLE_WARNINGS_AS_ERRORS "interpret compiler warnings as errors" TRUE)
option(ENABLE_SANITIZERS "enable address, undefined, pointer compare and pointer subtract sanitizers" TRUE)
# NOTE: clang-tidy configuration is deferred to the environment's .clang-tidy config file
# TODO: we should probably put all the clang-tidy-related cmake code in a separate clang-tidy.cmake file and include it here...
option(ENABLE_CLANG_TIDY "enable clang tidy analysis tool" TRUE)

# include utilities
include(cmake/clang-tidy.cmake)
include(cmake/warnings.cmake)
include(cmake/sanitizers.cmake)

# NOTE: in order for ctest to find all the tests properly, we must call enable_testing() from this top-most cmake lists file
if(ENABLE_TESTING)
    enable_testing()
endif()

add_subdirectory(simulator)
