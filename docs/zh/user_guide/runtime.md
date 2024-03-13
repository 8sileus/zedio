# 运行时

## 创建
创建好运行时，调用block_on时，程序会将执行权交给运行时，此时block_on的协程就是主协程
- 使用静态方法create将创建一个默认配置的运行时
``` C++
auto main() -> int{
    Runtime::create()
        .block_on(first_coro());
    return 0;
}
```
- 使用options方法可以创建一个Builder，在Builder里面可以配置运行时的相关环境，默认如下：
``` C++
auto main() -> int{
    Runtime::options()  
        // 设置多少个工作线程
        .set_num_workers(std::thread::hardware_concurrency())  
        // 设置将多少次submit才真正提供
        .set_submit_interval(4)
        // 多久检查一次全局队列
        .set_check_gloabal_interval(61)
        // 多久检查一个I/O驱动
        .set_check_io_interval(61)
        // io_uring的大小
        .set_ring_entries(1024)
        // 是否开启内核sqpoll
        .set_io_uring_sqpoll(false)
        .build()
        .block_on(first_coro());
    return 0;
}

```
## 结束
在第一个协程执行完毕后，运行时将被结束。

