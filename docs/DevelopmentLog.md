# 设备监控上位机开发日志

## Day 1

**日期：2026-09-13**

---

## 一、今日开发目标

搭建设备监控上位机项目的基础代码框架，实现一个最小可运行的设备监控闭环，为后续实现真实设备通信、数据库存储、报警和历史数据查询等功能打基础。

---

## 二、今日完成内容

### 1. 搭建项目基础目录

对项目代码进行了初步模块划分：

```text
DeviceMonitor
├── main.cpp
├── MainWindow.h
├── MainWindow.cpp
├── MainWindow.ui
│
├── Core
│   ├── DeviceData.h
│   ├── Device.h
│   ├── Device.cpp
│   ├── DeviceManager.h
│   └── DeviceManager.cpp
│
├── Data
│
├── Communication
│
└── Utils
```

当前主要完成 `Core` 模块和主界面，其他模块目录暂作为后续功能扩展使用。

---

### 2. 完成 MainWindow 主界面

创建 Qt Widgets 主窗口：

- `MainWindow.h`
- `MainWindow.cpp`
- `MainWindow.ui`

使用 Qt Designer 设计基础监控界面，并添加 `QTableWidget` 作为设备数据展示区域。

当前表格包含以下字段：

```text
设备ID | 状态 | 温度 | 电压
```

---

### 3. 创建 DeviceData 数据结构

创建 `DeviceData`，用于统一保存设备当前监控数据：

```cpp
struct DeviceData
{
    double temperature = 0.0;
    double voltage = 0.0;
    bool isOnline = false;
};
```

通过数据结构统一管理设备数据，避免在模块之间传递大量独立参数，同时为后续增加其他设备参数预留扩展空间。

---

### 4. 创建 Device 设备类

创建 `Device` 类，用于表示单个设备。

目前 `Device` 负责：

- 保存设备 ID
- 保存设备当前数据
- 管理设备运行状态
- 更新设备数据
- 通过 Qt 信号通知数据发生变化

主要接口：

```cpp
void start();
void stop();
bool isRunning() const;
void updateData();
```

当前阶段暂未连接真实设备，使用 `QRandomGenerator` 模拟温度和电压数据。

---

### 5. 创建 DeviceManager 设备管理类

创建 `DeviceManager` 类，用于统一管理多个 `Device`。

主要功能：

- 添加设备
- 保存设备列表
- 批量更新设备
- 批量启动设备
- 批量停止设备
- 转发设备数据更新信号

当前创建了 3 个模拟设备：

```text
Device 1
Device 2
Device 3
```

形成：

```text
DeviceManager
├── Device 1
├── Device 2
└── Device 3
```

---

### 6. 使用 QTimer 实现周期性数据更新

在 `MainWindow` 中创建 `QTimer`，设置 1 秒更新一次设备数据。

数据更新流程：

```text
QTimer
   ↓
DeviceManager::updateAllDevices()
   ↓
Device::updateData()
   ↓
更新设备数据
```

实现了多个模拟设备的周期性数据刷新。

---

### 7. 使用 Qt 信号槽实现数据传递

建立设备数据从设备层到界面的传递机制：

```text
Device
   │
   │ dataUpdated
   ↓
DeviceManager
   │
   │ deviceDataUpdated
   ↓
MainWindow
   │
   ↓
QTableWidget
```

`Device` 不直接操作 UI，而是通过信号发送 `DeviceData`，由 `MainWindow` 接收并更新界面，实现了设备逻辑与 UI 的基本解耦。

---

### 8. 完成设备表格实时更新

`MainWindow` 根据设备 ID 确定对应表格行，并更新：

- 设备 ID
- 在线/离线状态
- 温度
- 电压

温度和电压通过：

```cpp
QString::number(value, 'f', 1)
```

转换为保留 1 位小数的字符串后显示。

最终实现：

```text
设备ID    状态    温度    电压
--------------------------------
1         在线    35.2    24.1
2         在线    42.7    23.8
3         在线    31.5    24.4
```

数据能够随定时器周期自动变化。

---

## 三、开发过程中遇到的问题

### 设备列表没有正常更新

最初添加设备时使用了：

```cpp
deviceManager->devices().append(device1);
```

但 `devices()` 返回的是 `QList<Device*>` 的副本，因此对返回结果调用 `append()` 并不会修改 `DeviceManager` 内部真正保存的 `m_devices`。

导致：

```text
QTimer
 ↓
DeviceManager::updateAllDevices()
 ↓
m_devices为空
 ↓
没有设备被更新
```

### 问题解决

改为使用：

```cpp
deviceManager->addDevice(device1);
deviceManager->addDevice(device2);
deviceManager->addDevice(device3);
```

通过 `addDevice()` 将设备加入 `DeviceManager` 内部列表，并建立设备数据更新信号的连接。

修改后，三个模拟设备能够正常更新并刷新主界面。

---

## 四、当前项目状态

第一天已经完成最基础的设备监控数据链路：

```text
QTimer
  ↓
DeviceManager
  ↓
Device
  ↓
DeviceData
  ↓
Qt Signal / Slot
  ↓
MainWindow
  ↓
QTableWidget
```

当前设备数据仍采用随机数据模拟，尚未接入真实设备。

---

## 五、今日主要学习内容

- Qt Widgets 项目基本结构
- Qt Designer
- `QMainWindow`
- `QTableWidget`
- `QObject`
- `Q_OBJECT`
- Qt Signal / Slot
- `QTimer`
- `QList`
- C++ 类封装与对象管理
- Qt 对象父子关系
- `QString::number()`
- 基础模块解耦思想

---

## 六、下一步开发计划

下一阶段继续完善设备管理功能，并逐步向完整设备监控系统扩展：

1. 完善设备启动/停止及在线/离线状态管理
2. 增加设备详细信息显示
3. 引入 SQLite 保存设备历史数据
4. 实现历史数据查询
5. 增加实时数据曲线
6. 学习并接入串口通信
7. 根据实际需求引入多线程
8. 增加设备报警与日志功能

当前阶段优先保持现有基础架构稳定，不提前实现后续模块。