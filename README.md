# ZEDLIBS 
[![C++](https://img.shields.io/badge/language-C++-blue.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/platform-linux%20-lightgrey.svg)](https://www.kernel.org/)  

ZED is a modern C++ 基础工具库的集合,目前已包含:异步IO,日志,其余功能正在开发中。目前采用head only模式
```
 ________      _______       ________     
|\_____  \    |\  ___ \     |\   ___ \      
 \|___/  /|   \ \   __/|    \ \  \_|\ \        
     /  / /    \ \  \_|/__   \ \  \ \\ \  
    /  /_/__    \ \  \_|\ \   \ \  \_\\ \   
   |\________\   \ \_______\   \ \_______\  
    \|_______|    \|_______|    \|_______|  
```
# 操作系统环境
Ubuntu 22.10
理论上Linux内核版本在5.1以上应该都可以运行。
# 如何使用
## 编译器
- g++ 13.1.0 以上;
## 安装以及编译
ZED是header-only的工具库，你可以直接cp zed文件夹到你的项目中。或者通过CMake安装。
1. clone zedlibs
```
gitclone https://github.com/8sileus/zedlibs
cd zedlibs
mkdir build
cd build
```
2. install
```
cmake --install . # --prefix ./user_defined_install_path 
```
3. compile tests
```
cmake --build . # -j num_thread
ctest .
```

# 依赖库
boost    
liburing: https://github.com/axboe/liburing
# 项目介绍
## asynchronous I/O


## log

