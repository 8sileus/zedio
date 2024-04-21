# Runtime

## Start
- Use the default configuration
``` C++
auto main() -> int{
    Runtime<>::create()
        .block_on(first_coro());
    return 0;
}
```
- Use the specified configuration
``` C++
    // use Runtime<>::create() like this
auto main() -> int{
    Runtime<>::options()
        // number of worker threads 
        .set_num_workers(std::thread::hardware_concurrency())  
        // The maximum number of submissions can be delayed
        .set_num_weak_submissions_(4)
        // Check the global queue interval
        .set_check_gloabal_interval(61)
        // Check the io driver interval
        .set_check_io_interval(61)
        // Size of io uring
        .set_ring_entries(1024)
        // Enable feature: io_uring_sqpoll
        .set_io_uring_sqpoll(false)
        .build()
        .block_on(first_coro());
    return 0;
}

```
## Stop
when the first coroutine completes, the program ends;
