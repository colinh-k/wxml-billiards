#!/usr/bin/env python3


import numpy as np
import matplotlib.pyplot as plt
import matplotlib as mpl
import argparse
import pathlib


def main(args: argparse.Namespace):
    grid_dims, velocities = read_velocities(args.filepath)
    nrows, ncols = grid_dims

    plt.style.use("dark_background")

    x_hex, y_hex = get_hex_lattice_coordinates(nrows, ncols)

    # x/y velocities at each of the hex lattice points
    velocity_magnitude = np.linalg.norm(velocities, axis=1, keepdims=True)
    # normalize so arrows are same size
    # divide by 1 instead of 0 if magnitude is 0
    velocities_normalized = velocities / np.where(
        velocity_magnitude == 0, 1, velocity_magnitude
    )

    # magnitude of velocity at each hex lattice point
    velocity_magnitudes = np.linalg.norm(velocities, axis=1).reshape(grid_dims)

    fig, ax = plt.subplots(1, 1)
    ax.quiver(
        x_hex.flatten()[:: args.keep_every],
        y_hex.flatten()[:: args.keep_every],
        # velocities_normalized,
        velocities_normalized[:, 0].flatten()[:: args.keep_every],
        velocities_normalized[:, 1].flatten()[:: args.keep_every],
        # uncomment to color the arrows according to magnitude:
        # magnitude_hex,
        color="white",
        pivot="tip",
        angles="xy",
        scale=10,
        scale_units="inches",
        width=0.006,
        headlength=3.0,
        headaxislength=2.5,
        headwidth=3.5,
    )
    # NOTE: origin='lower' mirrors the image about its horizontal midpoint axis. plot the non-normalized vector arrows to see if the color map lines up with the arrows correctly. i believe 'lower' is the correct option
    color_map = ax.imshow(
        velocity_magnitudes,
        interpolation="gaussian",
        aspect="auto",
        origin="lower",
        extent=(
            x_hex.min(),
            x_hex.max(),
            y_hex.min(),
            y_hex.max(),
        ),
        cmap=mpl.colormaps["plasma"],
        norm="log",
    )

    ax.set_xlim(x_hex.min() - 0.5, x_hex.max() + 0.5)
    ax.set_ylim(y_hex.min() - 0.5, y_hex.max() + 0.5)

    ax.set_title(f"halfplane configuration ({nrows},{ncols})")
    ax.set_axis_off()

    fig.colorbar(
        color_map,
        location="right",
        label="magnitude of velocity",
        # fraction=0.1
    )

    # TODO: add quiverkey so viewers know each arrow is unit length

    plt.show()


# TODO: the binary data are all written in big endian order, so make sure to account for that when reading, ie convert from big endian to native format
# UPDATE: i tried reading as '>f8' for big endian 64bit float but the data was wrong... idk why reading as little endian works but it does...
def read_velocities(filepath: pathlib.Path):
    N_VELOCITY_COMPONENTS = 2

    with open(filepath, mode="rb") as file:
        # first, there are 2 uint64 values specifying (row,col) grid dimensions
        grid_dimensions = np.fromfile(file, dtype=">u8", count=2)
        # the rest is the raw velocity data as pairs of 64bit floating points
        velocities = np.fromfile(file, dtype="f8")

        nrows, ncols = grid_dimensions
        # the velocity vectors are stored in row-major order in the file. this is the 'C'-style order in numpy, where the last index changes fastest
        velocities = velocities.reshape((nrows * ncols, N_VELOCITY_COMPONENTS))
        # ball_velocities = raw_data.reshape((nrows, ncols, N_VELOCITY_COMPONENTS))

        # when the data file contains just the initial state of the system, this should be the only nonzero velocity
        # print(ball_velocities[nrows // 2, 0, :])

        return grid_dimensions, velocities


# given the dimensions of the grid, return a tuple of matrices which together index into the hexagonal lattice of the 2d packed ball layout of our simulation
def get_hex_lattice_coordinates(nrows, ncols):
    BALL_RADIUS = 1.0
    assert BALL_RADIUS == 1.0, (
        "some calculations below assume radius=1; in particular, the sqrt(3) would have to change if radius!=1"
    )

    # rectangular lattice coordinates
    y_rect, x_rect = np.mgrid[0:nrows, 0:ncols]
    # hex lattice coordinates
    x_hex = (x_rect + (y_rect % 2) * (BALL_RADIUS / 2.0)) * 2.0
    y_hex = y_rect * np.sqrt(3)

    return x_hex, y_hex


# argparse validator to check a path string is an accessible file that already exists
def existing_file(filepath_string: str) -> pathlib.Path:
    filepath = pathlib.Path(filepath_string)
    if not filepath.is_file():
        raise argparse.ArgumentTypeError(
            f"file path '{filepath_string}' does not exist or is not a file"
        )
    return filepath


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="visualize a saved simulation state for the halfplane configuration"
    )
    parser.add_argument(
        "filepath",
        type=existing_file,
        help="path to custom binary format simulator data file for halfplane configuration",
    )
    # only draw every this-many arrows in the velocity field. for large systems, the plot can look cluttered and be hard to read if we draw every arrow
    parser.add_argument(
        "-k",
        "--keep-every",
        type=int,
        default=7,
        help="keep every this-many velocity vectors when drawing the arrow field; this helps reduce visual clutter",
    )

    main(parser.parse_args())
