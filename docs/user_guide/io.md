# Asynchronous I/O

## Asynchronous call I/O 
``` C++
auto async_coawait(){
    char buf[1024];
    // use co_await syntax
    auto ret = co_await zedio::io::send(fd,buf,sizeof(buf));
    if(ret){
        ...
    } else {
        ...
    }
}
```

## Set expired_time
``` C++
    char buf[1024];
    // specify duration
    auto ret = co_await zedio::io::send(fd,buf,sizeof(buf)).set_timeout(3s);
    // specify time point
    auto deadline=std::chrono::steady_clock::now()+3s;
    auto ret = co_await zedio::io::send(fd,buf,sizeof(buf)).set_timeout_at(deadline):
```

