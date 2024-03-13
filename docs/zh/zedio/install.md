# Usage requirement

**平台**: Linux core>5.1  
**标准**: C++23   
**编译器**: GCC 13.1.0

+ [CMake](https://cmake.org)
+ [boost-test](https://github.com/boostorg/test)
+ [liburing](https://github.com/axboe/liburing)

::: tip VCPKG
vcpkg can be used to install dependencies.
:::

## 安装
- 复制文件夹
    ```ps
    cp -r zedio [user_defined_head_path]
    ```
- 使用CMake
    ```ps
    cmake --install . # --prefix ./user_defined_install_path 
    ```


## 编译测试文件

```ps
cmake -B build 
cd build 
cmake --build . # -j num_thread
ctest .
```
