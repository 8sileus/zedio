---
layout: home
title: Zedio

hero:
  name: Zedio
  text: Event-driven library for asynchronous applications in C++
  actions:
    - theme: brand
      text: Get Started
      link: /zedio/install
    - theme: alt
      text: View Benchmark
      link: /zedio/benchmark
    - theme: alt
      text: View on GitHub
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
