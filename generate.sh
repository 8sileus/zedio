#!/bin/bash


cwd=$(pwd)
echo $cwd

cmake -B ${cwd}/build && \
cd ${cwd}/build && \
make -j3 