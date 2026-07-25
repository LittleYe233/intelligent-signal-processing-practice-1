# 哈尔滨工业大学（深圳）《智能信号处理实践》课程题目1源码

此工程旨在提供一个存在噪声和干扰条件下实数正弦信号的频率估计算法仿真测试平台，并提供批量扫描参数测试。

## 编译

本工程为 Windows 平台设计，且开发编译平台均为 Windows。假定已经配置 MSYS2 环境并使用 UCRT64 平台的 shell。

首先安装编译工具链和环境：

```shell
pacman -Syu mingw-w64-ucrt-x86_64-clang mingw-w64-ucrt-x86_64-gdb mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-clang-tools-extra mingw-w64-ucrt-x86_64-cmake
```

然后安装必要的第三方库：

```shell
pacman -Syu mingw-w64-ucrt-x86_64-glfw mingw-w64-ucrt-x86_64-gettext-runtime mingw-w64-ucrt-x86_64-gettext-tools
```

克隆本仓库：

```shell
git clone https://github.com/LittleYe233/intelligent-signal-processing-practice-1.git
```

本仓库引用了其它仓库作为 submodules，需要同步：

```shell
git submodule update --init --recursive
```

本仓库在 CMake 中配置了多种编译变体：

```shell
# 通常开发，选用 Debug variant
cmake -DCMAKE_BUILD_TYPE:STRING=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -B build -G Ninja

# 构建生产环境版本，选用 Release variant 并为当前机器 CPU 选择最佳优化
cmake -DISPP_ENABLE_NATIVE=ON -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -B build -G Ninja
```

CMake configure 之后，可以开始编译：

```shell
cmake --build build
```

## 技术栈

- 编程语言和编译框架：C++20 & CMake
- 图形化界面和绘图：ImGUI & ImPlot
- 底层图形驱动：OpenGL3 & GLFW
- 线性代数数学库：Eigen
- 简易FFT计算库：PocketFFT
- 国际化：GNU Gettext

## 许可证

[MIT License](/LICENSE)