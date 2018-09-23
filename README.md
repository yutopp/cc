# cc

`cc` is a C compiler for my study purpose. A aim of this project is to create a C compiler which is modularized, no memory leaks, easy to modify codes and able to self hosting! Currently, highly work in progress.

This compiler has 5 phases to generate an assembly.

- Lexing
- Parsing
- Analyzing
- IR generating
- ASM generating

## How to build

``` shell
> make
```

## How to use

``` shell
> ./cc examples/simple_00.c
```

Currently, a name of the generated executable is fixed to `a.out`.

```
> ./a.out
Hello world
```

# Author

@yutopp
