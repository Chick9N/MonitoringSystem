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


## Day 2

**日期：2026-09-14**

---

## 一、开发目标

在第一天完成设备数据模拟、设备管理以及主界面设备总览的基础上，实现设备详情查看功能。

主要目标：

- 创建 DeviceWidget 设备详情组件
- 实现从设备总览进入设备详情
- 实现设备数据实时同步显示
- 完善设备详情窗口生命周期管理

---

# 二、完成内容

## 1. 增加 DeviceManager 设备查询功能

新增接口：

```cpp
Device* getDevice(int deviceId);
```

用于根据设备 ID 获取对应的 Device 对象。

实现逻辑：

```text
输入设备ID
    ↓
遍历 DeviceManager 管理的设备列表
    ↓
匹配 Device.id()
    ↓
返回对应 Device 指针
```

如果未找到设备：

```cpp
return nullptr;
```

该接口为后续通过 UI 操作指定设备提供基础。

---

## 2. 创建并完善 DeviceWidget 设备详情组件

新增设备详情窗口：

```text
DeviceWidget
```

用于显示单个设备的详细信息。

显示内容：

- 设备编号
- 在线状态
- 温度
- 电压

界面示例：

```text
----------------------
设备编号：1

状态：在线

温度：35.2 ℃

电压：24.1 V
----------------------
```

---

## 3. 实现 DeviceWidget 与 Device 数据绑定

DeviceWidget 内部保存：

```cpp
Device* m_device;
```

表示一个 DeviceWidget 对应一个 Device。

数据更新流程：

```text
Device
  |
  | dataUpdated(DeviceData)
  ↓
DeviceWidget
  |
  ↓
updateWidget()
  |
  ↓
刷新界面
```

实现效果：

- Device 负责数据和业务逻辑
- DeviceWidget 负责数据显示
- UI 不直接修改设备数据

实现了业务层和显示层分离。

---

## 4. 实现 MainWindow 双击打开设备详情

在 MainWindow 中增加设备表格双击事件：

```cpp
on_deviceTable_cellDoubleClicked()
```

实现流程：

```text
用户双击设备
        ↓
获取设备所在行
        ↓
转换为设备ID
        ↓
DeviceManager::getDevice()
        ↓
获取 Device 对象
        ↓
创建 DeviceWidget
        ↓
显示详情窗口
```

实现：

```text
设备总览界面

        ↓ 双击

对应设备详情窗口
```

---

## 5. 实现设备详情窗口唯一管理

### 遇到问题

重复双击同一个设备会创建多个详情窗口：

```text
Device 1

第一次点击
    ↓
DeviceWidget1


第二次点击
    ↓
DeviceWidget2
```

产生多个相同设备窗口。

---

### 解决方案

使用：

```cpp
QMap<int, DeviceWidget*> m_deviceWidgets;
```

保存已经打开的设备窗口。

数据结构：

```text
设备ID        DeviceWidget

1        →    DeviceWidget1

2        →    DeviceWidget2

3        →    DeviceWidget3
```

打开设备详情前：

```cpp
m_deviceWidgets.contains(deviceId)
```

判断窗口是否已经存在。

如果存在：

- 不重新创建
- 激活已有窗口

---

## 6. 解决关闭窗口后无法重新打开问题

### 问题

关闭设备详情窗口后：

```text
DeviceWidget 已经关闭

但是：

QMap 中仍保存旧指针
```

导致：

再次打开时：

```cpp
contains(deviceId)
```

仍然返回 true。

程序认为窗口存在，但实际上窗口已经销毁。

---

### 解决方案

设置窗口关闭自动销毁：

```cpp
setAttribute(Qt::WA_DeleteOnClose);
```

监听对象销毁信号：

```cpp
QObject::destroyed
```

窗口销毁时：

```cpp
m_deviceWidgets.remove(deviceId);
```

完整生命周期：

```text
创建 DeviceWidget

        ↓

加入 QMap 管理

        ↓

用户关闭窗口

        ↓

对象销毁

        ↓

触发 destroyed 信号

        ↓

移除 QMap 中记录

        ↓

可以重新创建窗口
```

---

# 三、遇到的问题及解决

## 问题1：重复打开设备详情窗口

### 原因

没有记录已经打开的窗口。

### 解决

使用：

```cpp
QMap<int, DeviceWidget*>
```

建立：

```text
设备ID → 窗口对象
```

映射关系。

---

## 问题2：关闭窗口后无法重新打开

### 原因

窗口销毁后，管理列表仍保存旧指针。

### 解决

使用：

```cpp
Qt::WA_DeleteOnClose
```

配合：

```cpp
destroyed
```

信号。

关闭窗口时自动清理记录。

---

# 四、当前项目架构

当前结构：

```text
MainWindow

    |
    |
    | 设备总览
    |
    ↓

DeviceManager

    |
    |
    ├── Device 1
    │       |
    │       ↓
    │   DeviceWidget 1
    |
    |
    ├── Device 2
    │       |
    │       ↓
    │   DeviceWidget 2
    |
    |
    └── Device 3
            |
            ↓
        DeviceWidget 3
```

数据流：

```text
Device

 ↓ dataUpdated

DeviceWidget

 ↓

界面刷新
```

---

# 五、今日学习内容

- Qt QWidget 子窗口管理
- Qt 信号槽跨对象通信
- QObject 生命周期管理
- destroyed 信号使用
- Qt::WA_DeleteOnClose 属性
- QMap 对象映射管理
- Lambda 捕获机制

---

# 六、当前完成度

已完成：

✅ 多设备模拟管理

✅ 主界面设备总览

✅ 设备详情窗口

✅ Device 与 DeviceWidget 数据同步

✅ 双击打开设备详情

✅ 详情窗口唯一管理

✅ 窗口生命周期管理


未完成：

- 设备启动/停止控制
- SQLite 数据存储
- 历史数据查询
- 串口通信
- 多线程数据采集
- 报警系统

---

# 七、下一步计划

## Day 3

计划实现：

1. DeviceWidget 增加设备控制按钮
2. 实现启动/停止设备功能
3. 完善设备状态变化逻辑
4. 引入 SQLite 数据库存储设备数据
5. 实现历史数据查询功能
