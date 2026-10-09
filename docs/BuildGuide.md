# 构建与运行

## 构建环境

- Qt 6.8.1 MinGW 64-bit
- CMake 3.16 或更新版本
- Ninja 和与 Qt Kit 配套的 MinGW 编译器
- Qt Widgets、Sql、Charts、SerialPort、Network、Concurrent 模块

在 Qt Creator 中打开项目根目录的 `CMakeLists.txt`，选择 Desktop Qt 6.8.1 MinGW 64-bit Kit 后执行构建。

也可以在已配置好 Qt/MinGW 环境的终端执行：

```powershell
cmake -S . -B build/Desktop_Qt_6_8_1_MinGW_64_bit-Debug -G Ninja `
  -DCMAKE_PREFIX_PATH=D:/qt/6.8.1/mingw_64 `
  -DCMAKE_MAKE_PROGRAM=D:/qt/Tools/Ninja/ninja.exe `
  -DCMAKE_CXX_COMPILER=D:/qt/Tools/mingw1310_64/bin/g++.exe
cmake --build build/Desktop_Qt_6_8_1_MinGW_64_bit-Debug --parallel 4
```

## 运行数据

- SQLite 数据库文件名为 `device.db`，路径相对于应用当前工作目录。Qt Creator 中应将工作目录设为项目根目录，避免调试运行和独立运行意外创建两份数据库。
- 日志位于 Qt `AppLocalDataLocation/logs/monitoring.log`，主界面的“查看日志目录”按钮可以打开目录。
- 删除设备会删除该设备的配置、历史采样和报警记录；新设备优先使用最小可用 ID。

## Modbus 数据约定

当前采集端按三个连续保持寄存器解释数据：

| 偏移 | 含义 | 转换 |
| --- | --- | --- |
| 0 | 温度 | 原始整数除以 10，单位 ℃ |
| 1 | 电压 | 原始整数，单位 V |
| 2 | 在线标志 | `1` 在线，其他值离线 |

RTU 从站地址和 TCP Unit ID、起始地址、读取数量在设备配置中指定。当前设备轮询器使用单个串行总线和一个 Modbus TCP 客户端；TCP 设备应配置到同一 IP/端口端点。
