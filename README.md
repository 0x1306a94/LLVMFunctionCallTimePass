# [自定义 LLVM PASS 实现 函数耗时插桩统计](https://blog.0x1306a94.com/docs/llvm/ch01/01/)
* 使用本仓库
* 需要安装`cmake`

```shell
# 首先创建一个目录 例如 LLVM
mkdir LLVM
cd LLVM

wget https://raw.githubusercontent.com/0x1306a94/LLVMFunctionCallTimePass/master/setup.sh -O setup.sh

wget https://raw.githubusercontent.com/0x1306a94/LLVMFunctionCallTimePass/master/build.sh -O build.sh

chmod +x ./setup.sh ./build.sh
./setup.sh
./build.sh -b -x
```
