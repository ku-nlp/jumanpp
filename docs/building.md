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