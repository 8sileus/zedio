# Performance comparison Tokio
OS：Ubuntu23.04  
Number of cores ：4    
Memory：4G  
CPU：AMD Ryzen 5 3600 6-Core Processor  
command：./wrk -t4 -c1000 -d90s --latency
ZEDIO:  
![](./images/zedio_benchmark.png)  
TOKIO:  
![](./images/tokio_benchmark.png)  