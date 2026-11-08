#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build
clang++ -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter -I/opt/homebrew/include -Isrc src/main.cpp src/base/stb_image.cpp -L/opt/homebrew/lib -lGLEW -lglfw -lassimp -framework OpenGL -o build/laboratorio
