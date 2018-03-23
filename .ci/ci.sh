#!/bin/bash

git submodule update --init --recursive
mkdir -p build/debug
cd build/debug
cmake ../.. -GNinja -DANTHEM_BUILD_TESTS=ON
ninja anthem-app && ninja run-tests
