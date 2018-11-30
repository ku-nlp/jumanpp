# How to build on other OS

## CentOS / RedHat EL Family

The default compiler and CMake are ancient.
You would need to get a modern compiler.
The easiest way is to install them is to use 
[Software Collections](https://developers.redhat.com/products/softwarecollections/overview/).
Devtoolset from them enables to build binaries which are runnable on machines without
devtoolset installed.

Here is the instruction for CentOS:


```bash
# Install Software Collections and devtoolset (need to do only once)
sudo yum install centos-release-scl
sudo yum install devtoolset-7 llvm-toolset-7-cmake
scl enable devtoolset-7 llvm-toolset-7 bash
# Following lines should be typed from the build directory
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j
make test
```

# Non-Cmake builds

Some IDEs provide builtin support for CMake projects
and do not require manual installation of CMake.

## Visual Studio

Starting since 2017 version, Visual Studio is capable to build CMake projects on its own.

Before attempting, be sure to install component `Visual C++ tools for CMake`

With that, you can open folder with jumanpp using `File->Open->Folder`

By default it builds x86, but it is recommended to use x64 builds for release

For more information on how to use CMake projects, follow [link](https://docs.microsoft.com/en-us/cpp/ide/cmake-tools-for-visual-cpp?view=vs-2017)
