# wgtcc
a small C11 compiler in C++11

## ENVIRONMENT
  1. x86-64
  2. linux 4.4.0
  3. gcc 5.4.0 (or any version supports C++11)  

## INSTALL
  ```bash
  $ make install # root required
  $ make test
  ```
  then you can play with the examples:
  ```bash
  $ wgtcc example/heart.c
  $ wgtcc example/chinese.c
  ```

## GOAL
**wgtcc** is aimed to implement the full C11 standard with some exceptions:

1. some features are supported only in grammar level(like keyword _register_)
2. features that disgusting me are removed(like default _int_ type without type specifier)
3. some non standard GNU extensions are supported, but you should not rely on **wgtcc** of a full supporting

## FRONT END
A basic recursive descent parser 

## BACK END
**wgtcc** generates code from AST directly. The algorithm is TOSCA(top of stack caching). It is far from generating efficient code, but at least it works and generates code efficently.

## MEMORY MANAGEMENT
Through **wgtcc** was wirtten in C++, i paid no effort for memory management except for a simple memory pool to accelerate allocations. _only_ _new_ is preferred because **wgtcc** runs fast and  exits immediately
after finishing parsing and generating code.

## REFERENCE
1. Compilers Principles, Techniques and Tools. second Edition.
2. [N1548, C11 standard draft](http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1548.pdf)
3. [64-ia-32-architectures-software-developer-manual-325462](http://www.intel.com/content/www/us/en/architecture-and-technology/64-ia-32-architectures-software-developer-manual-325462.html)
3. [8cc](https://github.com/rui314/8cc)
4. [macro expansion algorithm](https://github.com/wgtdkp/wgtcc/blob/master/doc/cpp.algo.pdf)

## TODO
1. support type qualification
2. support variable length array
