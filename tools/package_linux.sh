#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
sh tools/build_linux.sh
python3 tools/package_linux.py
