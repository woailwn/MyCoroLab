# benchmark

接下来演示如何运行性能测试，注意shell代码里用`init_dir`指示该shell初始的path，`tinycoro_path`表示tinycoro项目根目录的路径

## 测试工具准备

安装cargo

```shell
sudo apt-get install cargo
```

编译测试工具

```shell
# init_dir=tinycoro_path/build
make build-benchtools 
```

## 运行测试

由于压测工具与待测试程序的构建分离，需要运行下面的指令构建待测试程序

```shell
# init_dir=tinycoro_path/build
make build-bench
```

tinycoro提供了5种服务端模型供压测

- `bench_tinycoro_128`
- `bench_tinycoro_1k`
- `bench_tinycoro_16k`
- `bench_epoll_server` // 暂时废弃
- `bench_uring_server` // 暂时废弃

这里以负载1k，并发量为10为样例，用户开启两个终端分别为服务端和测试端

**step1.** 运行服务端程序，注意这里不用直接运行二进制文件，只需要`make`后跟上述的服务端模型的名字就行，服务端程序默认使用配置文件中的端口号

```shell
# init_dir=tinycoro_path/build
make bench_tinycoro_1k
```

**step2.** 运行测试端工具

```shell
# init_dir=tinycoro_path
cd benchmark
bash bench.sh 8000 10 1024
```

注意`bench.sh`需要接收三个参数，端口号、并发量和负载大小，负载大小默认以byte为单位。

## 运行baseline模型

与tinycoro作对比的基线模型为rust_echo_server，读者若想运行该基线模型可按下列步骤操作。

克隆rust_echo_server至本地

```shell
git clone https://github.com/haraldh/rust_echo_server.git
```

编译并运行

```shell
cd rust_echo_server
cargo run --release
```

注意rust_echo_server在本地开启监听的端口号为12345，对其进行压测的步骤与前述相同。
