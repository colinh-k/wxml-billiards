#!/usr/bin/env python3

import json
import itertools
import argparse
import pathlib

# since writing cmake presets by hand is quite repetitive (ie i have to specify build type for each platform), i decided to write this program to generate all of the various presets. the way i prefer writing presets follows a simple template, so it is easy to just generate all the platform/build type/toolchain/compiler options combinations


def main(args: argparse.Namespace):
    base_config = {
        "name": "base-config",
        "hidden": True,
        "generator": "Ninja",
        "binaryDir": "${sourceDir}/build/${presetName}",
        "toolchainFile": "${sourceDir}/cmake/toolchains/homebrew-clang.cmake",
        "cacheVariables": {
            "CMAKE_EXPORT_COMPILE_COMMANDS": "ON",
            "CMAKE_CXX_STANDARD": "23",
            "CMAKE_CXX_STANDARD_REQUIRED": "ON",
            "CMAKE_CXX_EXTENSIONS": "OFF",
            "CMAKE_COLOR_DIAGNOSTICS": "ON",
            # NOTE: the prefix path here is only useful for macos. specifically, its useful to tell cmake where to find clang-tidy
            "CMAKE_PREFIX_PATH": "$env{HOMEBREW_PREFIX}/opt/llvm/bin/",
            "ENABLE_CLANG_TIDY": False,
            "ENABLE_TESTING": True,
            "ENABLE_SANITIZERS": True,
            # runtime arguments for main
            "RESULTS_DIRPATH": "data/version_04/halfplane/",
            # compile-time arguments for main
            # "NROWS": "$env{NROWS}",
            # "NCOLS": "$env{NCOLS}",
            # "NROWS": "20",
            # "NCOLS": "20",
        },
    }
    base_build = {"name": "base-build", "hidden": True, "jobs": 4}
    base_test = {
        "name": "base-test",
        "hidden": True,
        "output": {"outputOnFailure": True},
        "execution": {"noTestsAction": "error", "stopOnFailure": True},
    }

    # specify the various configuration combinations
    build_types = ("Debug", "Release", "RelWithDebInfo")
    build_type_names = ("debug", "release", "relwithdebinfo")

    configure_presets = [base_config]
    build_presets = [base_build]
    test_presets = [base_test]
    workflow_presets = []

    for build_type, build_type_name in zip(build_types, build_type_names):
        name_str = f"{build_type_name}"

        configure_preset = {
            "name": name_str,
            "inherits": "base-config",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": f"{build_type}",
            },
        }
        # disable sanitizers for release builds
        if build_type_name in ("release", "relwithdebinfo"):
            configure_preset["cacheVariables"]["ENABLE_SANITIZERS"] = False
        configure_presets.append(configure_preset)

        build_preset = {
            "name": name_str,
            "inherits": "base-build",
            "configurePreset": name_str,
        }
        build_presets.append(build_preset)

        test_preset = {
            "name": name_str,
            "inherits": "base-test",
            "configurePreset": name_str,
        }
        test_presets.append(test_preset)

        workflow_preset = {
            "name": name_str,
            "steps": [
                {"type": "configure", "name": name_str},
                {"type": "build", "name": name_str},
                {"type": "test", "name": name_str},
            ],
        }
        workflow_presets.append(workflow_preset)

        # run workflow and run target for this build type
        build_presets.append(
            {
                "name": f"{build_type_name}-run",
                "inherits": "base-build",
                "configurePreset": f"{build_type_name}",
                "targets": "run-main",
            }
        )
        workflow_presets.append(
            {
                "name": f"{build_type_name}-run",
                "steps": [
                    {"type": "configure", "name": build_type_name},
                    {"type": "build", "name": f"{build_type_name}-run"},
                ],
            }
        )

    # assemble the final preset json object
    presets_object = {
        "version": 10,
        "configurePresets": configure_presets,
        "buildPresets": build_presets,
        "testPresets": test_presets,
        "workflowPresets": workflow_presets,
    }

    presets_json = json.dumps(presets_object, indent=4)

    if args.filepath:
        with args.filepath.open("w") as file:
            file.write(presets_json)

        print(
            f"finished generating cmake presets.\nwriting to {args.filepath.resolve()}"
        )
    else:
        print(f"generated presets string: (not saved to file)\n\n{presets_json}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="generate a CMake preset file with the given name"
    )
    parser.add_argument(
        "-f",
        "--filepath",
        type=pathlib.Path,
        help="if passed, actually perform the write to the file, overwriting any existing data in that file. otherwise, print the generated preset json object to the console",
    )
    main(parser.parse_args())
