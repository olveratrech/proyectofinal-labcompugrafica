#!/bin/sh
set -eu
cd "$(dirname "$0")/.."

build_dir=${BUILD_DIR:-build-linux}
cmake -S . -B "$build_dir" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_RPATH='$ORIGIN/lib' \
    -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON
cmake --build "$build_dir" --parallel

echo "Ejecutable: $build_dir/laboratorio"
