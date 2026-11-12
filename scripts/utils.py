import pathlib
import argparse


def positive_integer(value) -> int:
    try:
        value = int(value)
        assert value > 0
    except (ValueError, AssertionError):
        raise argparse.ArgumentTypeError(f"{value} is not a positive integer")

    return value


def existing_directory_string(dirpath_string: str) -> str:
    dirpath = pathlib.Path(dirpath_string)
    if not dirpath.is_dir():
        raise argparse.ArgumentTypeError(
            f"directory path '{dirpath_string}' does not exist or is not a directory"
        )
    return str(dirpath)


def non_existing_file_string(filepath_string: str) -> str:
    filepath = pathlib.Path(filepath_string)
    if filepath.exists():
        raise argparse.ArgumentTypeError(
            f"path '{filepath_string}' exists; expected a nonexistent filepath"
        )
    return str(filepath)
