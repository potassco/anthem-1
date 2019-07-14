#!/bin/bash

if [ "$1" = "gcc" ]
then
	cxx=g++
	cc=gcc
elif [ "$1" = "clang" ]
then
	cxx=clang++
	cc=clang
fi

git submodule update --init --recursive
mkdir -p build/debug
cd build/debug
cmake ../.. -GNinja -DCMAKE_BUILD_TYPE=Debug -DANTHEM_BUILD_TESTS=ON -DCMAKE_CXX_COMPILER=${cxx} -DCMAKE_C_COMPILER=${cc}
ninja anthem-app && ninja run-tests
