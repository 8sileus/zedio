# Usage requirement

**platform**: Linux core>5.1  
**standard**: C++23   
**compiler**: GCC 13.1.0

+ [CMake](https://cmake.org)
+ [boost-test](https://github.com/boostorg/test)
+ [liburing](https://github.com/axboe/liburing)

::: tip VCPKG
vcpkg can be used to install dependencies.
:::

## Install
- Copy zedio 
    ```ps
    cp -r zedio [user_defined_head_path]
    ```
- Use cmake
    ```ps
    cmake --install . # --prefix ./user_defined_install_path 
    ```


## Compile tests

```ps
cmake -B build 
cd build 
cmake --build . # -j num_thread
ctest .
```
