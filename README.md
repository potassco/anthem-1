# anthem [![GitHub Release](https://img.shields.io/github/release/potassco/anthem.svg?maxAge=3600)](https://github.com/potassco/anthem/releases) [![Build Status](https://img.shields.io/travis/potassco/anthem/master.svg?maxAge=3600&label=build%20%28master%29)](https://travis-ci.org/potassco/anthem?branch=master) [![Build Status](https://img.shields.io/travis/potassco/anthem/develop.svg?maxAge=3600&label=build%20%28develop%29)](https://travis-ci.org/potassco/anthem?branch=develop)

> Translate answer set programs to first-order theorem prover language

## Overview

`anthem` translates ASP programs (in the input language of [`clingo`](https://github.com/potassco/clingo)) to the language of first-order theorem provers such as [Prover9](https://www.cs.unm.edu/~mccune/mace4/).

## Usage

```bash
$ anthem [--complete] [--simplify] file...
```

`--complete` instructs `anthem` to perform Clark’s completion on the translated formulas.
With the option `--simplify`, the output formulas are simplified by applying several basic transformation rules.

## Building

`anthem` requires [CMake](https://cmake.org/) for building.
After installing the dependencies, `anthem` is built with a C++17 compiler (GCC ≥ 7.3 or clang ≥ 5.0).

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
