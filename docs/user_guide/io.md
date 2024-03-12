# Asynchronous I/O

## Call 

```cpp
    char buf[1024];
    // use co_await syntax
    auto ret = co_await zedio::io::send(fd, buf, sizeof(buf));
    if (ret) {
        ...
    } else {
        ...
    }
```

## Set timeout

- Specify duration
```cpp
    auto ret = co_await zedio::io::send(fd, buf, sizeof(buf)).set_timeou_for(3s);
```
- Specify time point
```cpp
    auto deadline = std::chrono::steady_clock::now() + 3s;
    auto ret = co_await zedio::io::send(fd, buf, sizeof(buf)).set_timeout_at(deadline);
```