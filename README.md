# RZ

single-file public domain (or MIT licensed) libraries for C inspired by https://github.com/nothings/stb 

The idea behind single-header file libraries is that they're easy to distribute and deploy because all the code is contained in a single file. By default, the .h files in here act as their own header files, i.e. they declare the functions contained in the file but don't actually result in any code getting compiled.
So in addition, you should select exactly one C/C++ source file that actually instantiates the code, preferably a file you're not editing frequently. This file should define a specific macro (this is documented per-library) to actually enable the function definitions. For example, to use rz_str, you should have exactly one C/C++ file that doesn't include rz_str.h regularly, but instead does
```c
#define RZ_STR_IMPLEMENTATION
#include "rz_str.h"
```

The right macro to define is pointed out right at the top of each of these libraries.

