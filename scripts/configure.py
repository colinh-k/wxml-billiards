#!/usr/bin/env python3

import argparse
import subprocess
import pathlib
import os

from utils import positive_integer, existing_directory_string


def main(args: argparse.Namespace) -> None:
    # apparently all env arguments must be strings
    new_env = dict(
        os.environ,
        NROWS=str(args.nrows),
        NCOLS=str(args.ncols),
        RESULTS_DIRPATH=args.directory_path,
    )
    try:
        # configure the build with our compile-time paramters
        subprocess.run(
            (
                "cmake",
                "--workflow",
                args.build_type,
            ),
            env=new_env,
            check=True,
        )
        if args.run:
            # run the built executable which should have all compile-time arguments baked into the cmake-generated build system
            subprocess.run(
                ("cmake", "--workflow", f"{args.build_type}-run"),
                env=new_env,
                check=True,
            )
    except subprocess.CalledProcessError as e:
        exit(e.returncode)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="builds and runs the 'main' simulation executable with the provided configuration parameters and output directory"
    )
    parser.add_argument(
        "-b",
        "--build_type",
        choices=("debug", "release", "relwithdebinfo"),
        default="release",
        help="the build type to build and run",
    )
    parser.add_argument(
        "-r",
        "--run",
        action="store_true",
        help="if passed, run the main executable after configuration and build succeeds",
    )
    parser.add_argument(
        "nrows", type=positive_integer, help="number of rows in the system"
    )
    parser.add_argument(
        "ncols", type=positive_integer, help="number of columns in the system"
    )
    parser.add_argument(
        "directory_path",
        type=existing_directory_string,
        help="path to a directory where the results file should be written. we must specify a path at configure-time, otherwise the custom cmake target will not pass any arguments to the executable",
    )

    main(parser.parse_args())


# artifacts from when this was going to be a bash script:
"""
#!/usr/bin/env bash

set -eu

# builds the main executable with the provided

usage() {
    cat << EOF
usage: "$0" [-h] [-b build_type] nrows ncols directory


args:
    - build_type is optional and defaults to 'release'. specifies what build type is built and run. must be one of 'debug', 'release', or 'relwithdebinfo'
    - nrows/ncols is the number of rows and columns in the simulation grid
    - directory is a path to a directory where the results file should be written
EOF
    exit 1
}

build_type="release"
while getopts 'hb:' opt
do
    case $opt in
        h) usage ;;
        b) build_type=$OPTARG ;;
    esac
done

shift $(( OPTIND - 1 ))
echo "hewow ${1}"
"""
