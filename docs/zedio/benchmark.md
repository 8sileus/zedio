# Benchmark

::: warning Results
These performance tests are not final; results vary from different test environments.
:::

## Server configuration

- OS：Ubuntu23.04  
- Number of cores: 4    
- Memory: 4G  
- CPU: AMD Ryzen 5 3600 6-Core Processor  

## Test tool

[wrk](https://github.com/wg/wrk)
```
./wrk -t4 -c1000 -d90s --latency
``` 

## Result

### ZEDIO:

![](/images/zedio_benchmark.png)

::: details ZEDIO
<<< @/../examples/benchmark.cpp
:::

### TOKIO:

![](/images/tokio_benchmark.png)

::: details TOKIO
<<< @/../benchmark/tokio_benchmark.rs
:::

###  MONOIO:

![](/images/monoio_benchmark.png)

::: details MONOIO
<<< @/../benchmark/monoio_benchmark.rs
:::

### GOLANG:

![](/images/golang_benchmark.png)

::: details GOLANG
<<< @/../benchmark/go_benchmark.go
:::
