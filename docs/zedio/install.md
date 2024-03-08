# Usage requirement

**platform**: Linux  
**standard**: c++23  

+ [CMake](https://cmake.org)
+ [boost-test](https://github.com/boostorg/test)
+ [liburing](https://github.com/axboe/liburing)

::: tip VCPKG
vcpkg can be used to install dependencies.
:::

## Build ctest

```ps
cmake --install . # --prefix ./user_defined_install_path 
cmake --build . # -j num_thread
ctest .
```
