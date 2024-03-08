---
layout: home
title: Zedio

hero:
  name: Zedio
  text: 一个C++异步运行时库
  actions:
    - theme: brand
      text: 快速开始
      link: /zh/zedio/install
    - theme: alt
      text: 基准测试
      link: /zh/zedio/benchmark
    - theme: alt
      text: 源码
      link: https://github.com/8sileus/zedio
---

+ Multithreaded, work-stealing based task scheduler. (reference [tokio](https://tokio.rs/))
+ Proactor event handling backed by [io_uring](https://github.com/axboe/liburing).
+ Asynchronous TCP and UDP sockets.

<style>
:root {
  --vp-home-hero-name-color: transparent;
  --vp-home-hero-name-background: -webkit-linear-gradient(120deg, #21fce3 30%, #95ed37);
}
</style>
