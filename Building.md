[Checkout](http://code.google.com/p/stinkhorn/source/checkout) | [Browse](http://code.google.com/p/stinkhorn/source/browse/) | [Changes](http://code.google.com/p/stinkhorn/source/list) | [Clones](http://code.google.com/p/stinkhorn/source/clones)

# Introduction #

Stinkhorn is configured through config.hpp, and buildable with g++ and visual C++. On windows, build using the visual studio solution provided. On anything else, `./configure; make`. Set CFLAGS in the configure script as you desire; the default is `-c -O3 -DNDEBUG -DB98_NO_64BIT_CELLS -DB98_NO_TREFUNGE`. If you add a new source file or a `#include` directive to an existing file, you will need to run `configure` again to regenerate the dependencies.

If you don't care about incremental building, you can also just do `g++ -O2 -DNDEBUG -o stinkhorn src/*.cpp`.

To get the source code, go to [the checkout page](http://code.google.com/p/stinkhorn/source/checkout) and check it out using mercurial, or use the source code browser.

# Options #

  * Define **DEBUG** to get a debug build. (Or, in the visual studio solution, select the debug configuration.)
  * Define **NDEBUG** to remove `assert()` calls from the executable.
  * Use **B98\_NO\_64BIT\_CELLS** to disallow 64-bit cells. This will decrease the executable size.
  * Use **B98\_NO\_TREFUNGE** to disallow trefunge source files. This will also decrease the executable size.
  * **B98\_WIN32\_DONT\_USE\_QPC** changes the HRTI timing method.