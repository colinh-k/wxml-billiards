# i want to use the simulator code in the main executable AND the test code. so we define an interface library so the sim source files can be shared across all targets
add_library(libsimulator INTERFACE)
target_sources(libsimulator
    INTERFACE
        FILE_SET
        HEADERS
        FILES
            random_index_set.hpp
            ball_grid.hpp
            vec2.hpp
            simulator.hpp
            bounded_small_set.hpp
)
target_compile_options(libsimulator
    INTERFACE
        # NOTE: after moving the neighbor sets claculation to compile time with constexpr functions, my compiler hit its limit for compile-time computation. this flag increases the limit so my (admittedly quite large) constexpr calculation completes
        # TODO: is there a cross-platform flag that works for gcc too ?
        -fconstexpr-steps=0
)

add_executable(main)
target_sources(main
    PRIVATE
        main.cpp
        # Ball.cpp
        # BallSimulator.cpp
        # HalfplaneArrangement.cpp
        # Vec2.cpp
)
target_link_libraries(main
    PRIVATE
        warnings
        sanitizers
        libsimulator
)
# for any private .hpp files in src directory
target_include_directories(main
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
# NOTE: the generator expression will still evaluate to false if the variable is 0, but that input is not interesting anyway
# target_compile_definitions(main
#     PRIVATE
#         $<$<BOOL:${NROWS}>:NROWS=${NROWS}>
#         $<$<BOOL:${NCOLS}>:NCOLS=${NCOLS}>
# )

target_compile_options(main
    PRIVATE
        # this flag is useful for profiling. without it, Instruments.app cant locate source code
        $<$<CONFIG:Debug,RelWithDebInfo>:-fno-omit-frame-pointer>

        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Debug>:-g3>

        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Release>:-g0>

        $<$<CONFIG:RelWithDebInfo>:-O2>
        $<$<CONFIG:RelWithDebInfo>:-g3>
)

if (ENABLE_CLANG_TIDY)
    target_enable_clang_tidy(main)
endif()

# symlink the project root's data directory into the build directory.
# normally, hard-coded relative paths in a c++ program are relative to the current working directory. when invoking the main target from the project root with cmake, the cwd is in the build directory. so any relative paths eg './data/my_data_file.dat' will complain the intermediary directory 'data/' does not exist
# introducing a symlink into the build directory solves this issue, so the c++ code can read/write to the top-level data/ directory
add_custom_command(
    TARGET
        main
    PRE_BUILD

    # COMMAND
    #     ${CMAKE_COMMAND} -E make_directory
    #         "$<TARGET_FILE_DIR:main>"
    COMMAND
        ${CMAKE_COMMAND} -E create_symlink
            "${PROJECT_SOURCE_DIR}/data"
            "$<TARGET_FILE_DIR:main>/data"
    COMMENT
        "creating symlink to top-level data directory"
)

add_custom_target(run-main
    DEPENDS
        main
    COMMAND
        # "$<TARGET_FILE:main>"
        "$<TARGET_FILE:main>" $<$<BOOL:RESULTS_DIRPATH>:${RESULTS_DIRPATH}>
    COMMENT
        "run the main application"
)

add_custom_target(run-debugger
    DEPENDS
        main
    COMMAND
        lldb "$<TARGET_FILE:main>"
    COMMENT
        "run debugger with the main application"
)
