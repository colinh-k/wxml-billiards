# configure clang-tidy
# TODO: perhaps make this not required. then only enable if found. if we do it that way, emit some kind of message to users can know if clang tidy was enabled in the build logs
if (ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY clang-tidy REQUIRED)
    set(CLANG_TIDY_COMMAND ${CLANG_TIDY})
endif()
# $(brew --prefix llvm)/bin/clang-tidy

# only enable clang tidy on specific targets
# see https://discourse.cmake.org/t/how-to-prevent-clang-tidy-to-check-sources-added-with-fetchcontents/2871/13
# NOTE: if clang_tidy_command is empty, then clang tidy is not enabled
# TODO: should i test if clang-tidy was enabled by the project-level cmake option inside this fn ? this would allow users of this fn to not have to check if clang-tidy was enabled, but it hides implementation details, and users might be confused if they didnt enable clang-tidy with the cmake option, but called this fn on a target expecting it to run...
function(target_enable_clang_tidy target)
    # TODO: perhaps we should add a check that the cmake option is enabled so we can inform the user what the issue is. otherwise, it might be hard for them to tell why clang-tidy wont work...
    set_target_properties(${target}
        PROPERTIES
            CXX_CLANG_TIDY
                "${CLANG_TIDY_COMMAND}"
    )
endfunction()
