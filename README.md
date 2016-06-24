lisp-forty
==========

Yet another Lisp

Created by following http://www.buildyourownlisp.com/

Current Features
----------------
* Q-Expressions
* Hash tables for variable lookup
* Builds with CMake
* Seperate types for booleans
* Compact standard library
* GPL'd

Planned Features
----------------
* Documentation
* Consistent style
* Tail-call optimization
* Pools for lvals

To build:

```bash
git submodule init
git submodule update
mkdir build
cd build
cmake ..
make
```
