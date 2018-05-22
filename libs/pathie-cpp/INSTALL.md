Installation
============

Pathie uses CMake as its build system, which you thus have to install
before you can compile Pathie. Other than that, you need a C++98
capable compiler.

The basic procedure is as follows:

    $ mkdir build
    $ cd build
    $ cmake -DCMAKE_BUILD_TYPE=Release ..
    $ make
    # make install

This will install Pathie below /usr/local. If that is not what you
want, specify a different path with the `CMAKE_INSTALL_PREFIX` option:

    $ cmake -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX=/where/you/want/it \
            ..

Other build options available:

PATHIE_ASSUME_UTF8_ON_UNIX=ON/OFF        Forces Pathie to assume that
                                         your filesystem encoding is
                                         UTF-8 instead of trying to
                                         determine it by looking at
                                         the environment of the
                                         programme.
                                         Default: OFF

PATHIE_BUILD_STREAM_REPLACEMENTS=ON/OFF  This enables the experimental
                                         replacements for std::ifstream
                                         and std::ofstream.
                                         Default: OFF

In order to use these options, pass them as `-DNAME_OF_OPTION=` with
either `ON` or `OFF` as the value when you invoke `cmake`.
