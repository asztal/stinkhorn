Stinkhorn is a befunge/trefunge-98 interpreter written in C++ which is mostly conformant to the befunge-98 specification and supports some of the fingerprints that are out there (HRTI, ROMA, NULL, TOYS, REFC, ORTH, MODU, ORTH, BOOL, SOCK, STRN). The handprint is 0x5E5F5F5E.

It supports befunge and trefunge, and cell sizes of 32 and 64 bits, both of which may be specified at run time. It is also possible to remove support for individual configurations at compile-time. See the usage section for details (pass the `--help` option to the interpreter).

It also includes a debugger (pass the `-d` option to the interpreter).

A warning: You probably don't want to use this. There are better and faster interpreters out there, and the code is old and crufty. Don't say I didn't warn you_!_