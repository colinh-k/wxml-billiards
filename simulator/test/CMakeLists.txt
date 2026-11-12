include(FetchContent)
FetchContent_Declare(Catch2
    GIT_REPOSITORY
        https://github.com/catchorg/Catch2.git
    # using commit hash instead of tag label lets cmake more reliably determine if it needs to redownload a library
    GIT_TAG
        317ac1ed4c0bb6e6b91eafc817e05c488feffcb3
    GIT_SHALLOW
        true
    # do not emit warnings about content in headers from this library (clang-tidy complains if not marked system)
    SYSTEM
)
FetchContent_MakeAvailable(Catch2)

add_executable(all-tests)
target_sources(all-tests
    PRIVATE
        random_index_set.cpp
        ball_grid.cpp
        bounded_small_set.cpp
)
target_link_libraries(all-tests
    PRIVATE
        warnings
        sanitizers
        libsimulator
        Catch2::Catch2WithMain
)
target_compile_definitions(all-tests
    PRIVATE
        # it seems clang 22 (?) issues a warning for catch2 TEST_CAST macro: "'__COUNTER__' is a C2y extension [-Wc2y-extensions]". idk why this happens but i think defining this macro causes catch2 to not use __COUNTER__, so the warning goes away
        CATCH_CONFIG_NO_COUNTER
)
if (ENABLE_CLANG_TIDY)
    target_enable_clang_tidy(all-tests)
endif()

# benchmarks are only available for release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")

    add_executable(benchmarks)
    target_sources(benchmarks
        PRIVATE
            benchmark.cpp
    )
    target_link_libraries(benchmarks
        PRIVATE
            warnings
            sanitizers
            libsimulator
            Catch2::Catch2WithMain
    )
    target_compile_definitions(benchmarks
        PRIVATE
            CATCH_CONFIG_NO_COUNTER
    )

    # useful to run the benchmark target
    add_custom_target(run-benchmarks
        DEPENDS
            benchmarks
        COMMAND
            "$<TARGET_FILE:benchmarks>"
        COMMENT
            "run the benchmarks executable"
        USES_TERMINAL
        VERBATIM
    )

    if (ENABLE_CLANG_TIDY)
        target_enable_clang_tidy(benchmarks)
    endif()

endif()

# NOTE: required since catch2 was included via fetchcontent
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(all-tests)
