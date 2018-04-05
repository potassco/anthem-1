# Change Log

## (unreleased)

### Features

* support for declaring placeholders with the `#external` directive

### Internal

* drops Boost dependency in favor of the header-only command-line option library [cxxopts](https://github.com/jarro2783/cxxopts)

## 0.1.6 (2017-06-12)

### Features

* unique IDs for all variables (user-defined variables are renamed)
* support for hiding predicates from completed output by using `#show` statements
* more simplification rules with `--simplify`
* command-line option `--parentheses` to fully parenthesize the output
* adds multiple example instances for experimenting

### Bug Fixes

* adds missing error message when attempting to read inaccessible file
* removes unnecessary parentheses after simplification
* fixes incorrect simplification with binary operations in arguments

## 0.1.5 (2017-05-04)

### Bug Fixes

* fixes lost signs with negated 0-ary predicates

## 0.1.4 (2017-04-12)

### Features

* completion of input programs (optional)
* command-line option `--complete` to turn on completion

## 0.1.3 (2017-03-30)

### Features

* support for anonymous variables

### Bug Fixes

* fixes incorrectly simplified rules with comparisons
* fixes misleading error message concerning negated, unsupported body literals

## 0.1.2 (2017-03-23)

### Features

* simplification of output formulas (optional)
* command-line option `--simplify` to turn on simplification

### Bug Fixes

* fixes incorrectly translated choice rules with multiple elements in the head aggregate

### Internal

* explicit syntax tree representation for first-order formulas

## 0.1.1 (2017-03-06)

### Features

* support for choice rules (without guards)

## 0.1.0 (2016-11-24)

### Features

* initial support for translating rules in *Essential Gringo* (excluding aggregates) to first-order logic formulas
* command-line option `--color` to autodetect, enable, or disable color output
* command-line option `--log-level` to control which status messages should be shown
