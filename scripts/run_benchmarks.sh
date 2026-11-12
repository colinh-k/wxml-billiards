#!/usr/bin/env bash

set -eu

# name of the file where the benchmark data will be saved without extension
FNAME=data/version_04/benchmarks/benchmark

# TODO: should we always build before running benchmarks ?
# cmake --workflow release

# NOTE: ${@:-} defaults to empty string if $@ is empty. if $@ is empty, then an error is generated due to 'set -eu' above
# NOTE: the builtin json reporter apparently does NOT store benchmark data.... so we must use xml
./build/release/simulator/test/benchmarks  \
    --reporter xml::out=${FNAME}.xml       \
    --reporter console::out=-              \
    ${@:-}
