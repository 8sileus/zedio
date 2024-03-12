# 异步I/O

## 调用方法
Zedio将异步I/O都封装成了一个Awaiter，所以只需要co_await语法就可以调用。同时在调用时必须保证参数的生命周期必须在其之前。

```cpp
char buf[1024];
// 记住不要忘记co_await
auto ret = co_await zedio::io::send(fd, buf, sizeof(buf));
if (ret) {
    ...
} else {
     ...
}
```

## 设置超时
Zedio只支持毫秒级定时，范围为[1毫秒——64**7毫秒)，如果设置了不在范围的数值，将返回错误。对于指定时间点，只能使用[std::chrono::steady_clock](https://en.cppreference.com/w/cpp/chrono/steady_clock)。

- 指定时长
```cpp
    auto ret = co_await zedio::io::send(fd, buf, sizeof(buf)).set_timeou_for(3s);
```
- 指定时间点
```cpp
    auto deadline = std::chrono::steady_clock::now() + 3s;
    auto ret = co_await zedio::io::send(fd, buf, sizeof(buf)).set_timeout_at(deadline);
```
