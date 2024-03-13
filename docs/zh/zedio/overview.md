# Zedio

```
  ______  ______   _____    _____    ____  
 |___  / |  ____| |  __ \  |_   _|  / __ \ 
    / /  | |__    | |  | |   | |   | |  | |
   / /   |  __|   | |  | |   | |   | |  | |
  / /__  | |____  | |__| |  _| |_  | |__| |
 /_____| |______| |_____/  |_____|  \____/ 
                                                                       
```

[![C++23](https://img.shields.io/static/v1?label=standard&message=C%2B%2B23&color=blue&logo=c%2B%2B&&logoColor=white&style=flat)](https://en.cppreference.com/w/cpp/compiler_support)
![Platform](https://img.shields.io/static/v1?label=platform&message=linux&color=dimgray&style=flat)


Zedio是采用C++23标准编写的异步运行时头文件库

## 特性
+ 多线程 + 任务窃取
+ 基于[io_uring](https://github.com/axboe/liburing)的I/O驱动
+ 零成本抽象

## 提供的异步服务
+ **I/O**
+ **网络** 
+ **文件系统** 
+ **时间** 
+ **同步工具** 
+ **日志** 

目前正在开发中，有兴趣的可以参与一下[贡献](contributing)
