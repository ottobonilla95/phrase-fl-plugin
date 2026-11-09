#!/bin/zsh
set -eu
cd "${0:A:h:h}"
if [[ ! -x .tools/bin/cmake ]]; then
    uv venv .tools
    uv pip install --python .tools/bin/python 'cmake==3.31.6' 'ninja==1.11.1.4'
fi
.tools/bin/cmake -S . -B build -G Ninja -DCMAKE_MAKE_PROGRAM="$PWD/.tools/bin/ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64
.tools/bin/cmake --build build --parallel 4
.tools/bin/ctest --test-dir build --output-on-failure
