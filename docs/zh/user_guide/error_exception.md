# 错误处理与异常

## 错误处理
Zedio采用[std::expected](https://en.cppreference.com/w/cpp/utility/expected)来包装可出错调用的返回值和错误。我认为这是一种优雅且严谨的处理方式。
## 异常
Zedio提供的所有函数都是不会抛出异常的，但是用户写的代码可能会抛出异常，Zedio根据编译时的模式选择不同的处理方式
- DEBUG：不管异常产生在何处，程序都会调用std::terminate()中断。
- RELEASE：如果异常并非是由主协程产生的，那么ZEDIO仅仅只是向std::cerr输出异常的内容。（TODO：在未来，Zedio会打印调用栈）