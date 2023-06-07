## 文件结构
```
├── Dockerfile
├── README.md
└── source
    ├── kendryte-toolchain-ubuntu-amd64-8.2.0-20190213.tar.gz
    ├── qemu-7.0.0-fixed.tar.xz
    ├── qemu-7.0.0.tar.xz
    ├── sources.list
    └── .devcontainer
        └── devcontainer.json
```

## 软件版本
镜像基于 Ubuntu:18.04

* sources.list
  * tuna ubuntu 18.04
* riscv-toolchain
  * [kendryte-gnu-toolchain](https://github.com/kendryte/kendryte-gnu-toolchain)
  * v8.2.0
* qemu
  * [qemu](https://qemu.org)
  * v7.0.0

## 直接下载

https://bhpan.buaa.edu.cn:443/link/67D5A8A6BCB587711FEE14256E3F3C6C
Valid Until: 2023-08-01 23:59

## 准备 source 文件
* sources.list
  * [tuna ubuntu 18.04](https://mirrors.tuna.tsinghua.edu.cn/help/ubuntu/)
* riscv-toolchain
  * [kendryte-toolchain-ubuntu-amd64-8.2.0-20190213.tar.gz](https://github.com/kendryte/kendryte-gnu-toolchain/releases/download/v8.2.0-20190213/kendryte-toolchain-ubuntu-amd64-8.2.0-20190213.tar.gz)
* qemu
  * qemu-7.0.0-fixed
    * [qemu-7.0.0.tar.xz](https://download.qemu.org/qemu-7.0.0.tar.xz)
    * 解压 qemu-7.0.0.tar.xz
    * 手动修改文件 `ebpf/ebpf_rss.c`
    * 将 52 行 `bpf_program__set_socket_filter(rss_bpf_ctx->progs.tun_rss_steering_prog);` 修改为 `bpf_program__set_type(rss_bpf_ctx->progs.tun_rss_steering_prog, BPF_PROG_TYPE_SOCKET_FILTER);`
    * 压缩为 qemu-7.0.0-fixed.tar.xz

## 安装位置

* riscv-toolchain
  * `opt/kendryte-toolchain`
* qemu
  * `opt/qemu`

## 相关命令

### 编译 image
```
docker build -t exaros .
```

### 运行 container
```
docker run -it -v /yourdir:/dockerdir exaros
```

### 导出 image
```
docker save -o exarosDockerImage.tar exaros:latest
```

### 导入 iamge
```
docker load < exarosDockerImage.tar
```

### help
```
docker help
```

## 使用 vscode 连接 docker

vscode 安装插件 `Dev Containers`

将 `source/.devcontainer` 文件夹移动到 `Exaros` 目录下

`Ctrl+Shift+P` 调用命令 `Dev Containers: Open Folder in Container..`，选择 `Exaros` 目录打开即可自动建立 container 并连接

## 在 docker 中使用 git

不推荐在 docker 中使用 ssh 而是使用 https 替代

### access token

在 github `settings/Developer settings/Personal access tokens/Tokens(classic)` 创建 personal access token

使用以下命令缓存 token

```
git config --global credential.helper store
```

此后只需要在认证 username 和 token 时输入一次即可自动缓存

## 在 docker 中使用 proxy

使用参数 `--network host`，虚拟机将直接使用宿主网络，设置 proxy 端口和宿主机一致即可

## 参考资料

* [Dev Container metadata reference](https://containers.dev/implementors/json_reference/)