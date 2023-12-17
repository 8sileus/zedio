# ZEDIO 
[![C++](https://img.shields.io/badge/language-C++-blue.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/platform-linux%20-lightgrey.svg)](https://www.kernel.org/)  

ZEDIO是一个异步运行时框架，用于使用C++编写异步应用程序。
```
  ______  ______   _____    _____    ____  
 |___  / |  ____| |  __ \  |_   _|  / __ \ 
    / /  | |__    | |  | |   | |   | |  | |
   / /   |  __|   | |  | |   | |   | |  | |
  / /__  | |____  | |__| |  _| |_  | |__| |
 /_____| |______| |_____/  |_____|  \____/ 
                                                                       
```
# 1.操作系统环境
ubuntu23.04
# 2.如何使用
## 2.1编译器
- g++ 13.1.0
## 2.2安装以及编译
zed 是 HEADER ONLY，你可以直接cp zed文件夹到你的项目中。或者通过CMake安装。
1. clone zedio
```
gitclone https://github.com/8sileus/zedio
cd zedio
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

# 3.项目介绍
## 3.1多线程，任务窃取机制
  zedio的多线程和任务窃取模型，参考自tokio。

## 3.2异步IO
- 对所有的I/O操作进行封装，提供异步调用的接口，该接口会先以非阻塞IO的方式调用原始的IO接口，如果还没有事件到达，将会把当前的coroutine注册进iouring，然后放弃执行权。（如果当前没有io_uring_sqe可用，则将把coroutine推送到等待队列，调度器空闲时处理这些等待的coroutine。）
## 3.3统一错误处理
- 所有的错误，都使用std::error_code存储。如std::expected<int,std::error_code>，任何可能会出错的操作都使用std::expected或std::optional进行封装。
## 3.4log
- 异步输出
- 日志分级，日志文件滚动，终端着色。
- 输出支持终端与文件。


# 4.依赖库
liburing: https://github.com/axboe/liburing