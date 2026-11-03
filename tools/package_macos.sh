#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
sh tools/build_macos.sh
python3 tools/package_macos.py
