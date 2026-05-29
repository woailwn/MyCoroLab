#!/bin/bash
if [ "$#" -ne 3 ]; then
    echo "Error! need two paras"
    echo "Usage: <port> <number>  <length>"
    exit 1
fi

../third_party/rust_echo_bench/target/release/echo_bench --address "127.0.0.1:$1" --number $2 --duration 30 --length $3