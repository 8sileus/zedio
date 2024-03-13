# 运行时

## 创建
创建好运行时，调用block_on时，程序会将执行权交给运行时，此时block_on的协程就是主协程
- 使用静态方法create将创建一个默认配置的运行时
``` C++
auto main() -> int{
    return Runtime::create()
        .block_on(first_coro());
}
```
- 使用options方法可以创建一个Builder，在Builder里面可以配置运行时的相关环境，默认如下：
``` C++
auto main() -> int{
    return Runtime::options()
                .scheduler()
                    //指定工作线程数量
                    .set_num_workers((std::thread::hardware_concurrency()))
                    //指定多久检查一次全局队列
                    .set_check_gloabal_interval(61)
                    //指定多久检查一下I/O驱动
                    .set_check_io_interval(61)
                .driver()
                    //指定将多少个I/O才真正执行submit，0代表将所有I/O合并为一个submit，
                    .set_submit_interval(4)
                    //指定io_uring的flags
                    .set_custom_flags(0)
                    //指定io_uring的entries
                    .set_ring_entries(1024)
                .build()
                .block_on(first_coro());
}

```
## 结束
在第一个协程执行完毕后，运行时将被结束。

