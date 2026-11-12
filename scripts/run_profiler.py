#!/usr/bin/env python3

import argparse
import subprocess

from utils import non_existing_file_string


def main(args: argparse.Namespace) -> None:
    try:
        # configure the build with our compile-time paramters
        subprocess.run(
            (
                "xctrace",
                "record",
                "--template",
                "Time Profiler",
                "--target-stdout",
                "-",
                "--output",
                args.filename,
                "--launch",
                "./build/relwithdebinfo/simulator/src/main",
                "data/halfplane/version_02/",
            ),
            check=True,
        )
        if args.open:
            subprocess.run(("open", "-a", "Instruments", args.filename), check=True)
    except subprocess.CalledProcessError as e:
        exit(e.returncode)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="capture a profile/trace of the relwithdebinfo build of the main executable"
    )
    parser.add_argument(
        "-o",
        "--open",
        action="store_true",
        help="open the saved .trace file with Instruments.app",
    )
    parser.add_argument(
        "filename",
        type=non_existing_file_string,
        help="filename in which the .trace profile will be saved. must not already exist (we dont want to overwrite existing traces)",
    )

    main(parser.parse_args())
