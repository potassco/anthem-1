# anthem [![GitHub Release](https://img.shields.io/github/release/potassco/anthem.svg?maxAge=3600)](https://github.com/potassco/anthem/releases) [![Build Status](https://img.shields.io/travis/potassco/anthem/master.svg?maxAge=3600&label=build (master))](https://travis-ci.org/potassco/anthem?branch=master) [![Build Status](https://img.shields.io/travis/potassco/anthem/develop.svg?maxAge=3600&label=build (develop))](https://travis-ci.org/potassco/anthem?branch=develop)

> Translate answer set programs to first-order theorem prover language

## Overview

`anthem` translates ASP programs (in the input language of [`clingo`](https://github.com/potassco/clingo)) to the language of first-order theorem provers such as [Prover9](https://www.cs.unm.edu/~mccune/mace4/).

## Usage

```bash
$ anthem [--simplify] file...
```

With the option `--simplify`, output formulas are simplified by applying several basic transformation rules.

## Building

`anthem` requires [CMake](https://cmake.org/) and [Boost](http://www.boost.org/) for building.
After installing the dependencies, `anthem` is built with a C++14 compiler (GCC ≥ 6.1 or clang ≥ 3.8).

```bash
$ git clone https://github.com/potassco/anthem.git
$ cd anthem
$ git submodule update --init --recursive
$ mkdir -p build/release
$ cd build/release
$ cmake ../.. -DCMAKE_BUILD_TYPE=Release
$ make
```

## Contributors

* [Patrick Lühne](https://www.luehne.de)
