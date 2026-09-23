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



## Day 3 —— SQLite 数据持久化与数据库业务层

### 一、今日开发目标

完成设备监控上位机系统的 SQLite 数据层，实现设备运行数据的持久化存储，并建立基本的历史数据查询与管理能力。

今日主要工作：

- 集成 SQLite 数据库
- 完成数据库连接与数据表创建
- 实现设备运行数据持久化
- 实现设备历史数据查询
- 实现指定时间范围历史数据查询
- 实现最新设备数据查询
- 实现设备历史数据删除
- 完善数据库索引
- 打通设备实时数据到 SQLite 的数据链路
- 对数据库读写功能进行基本验证

---

## 二、SQLite 数据库集成

项目使用 Qt SQL 模块连接 SQLite 数据库。

数据库文件：

```text
device.db
```

数据库采用 SQLite，无需额外部署数据库服务器，适合当前设备监控上位机的单机应用场景。

数据库连接通过 `QSqlDatabase` 完成：

```cpp
m_database = QSqlDatabase::addDatabase("QSQLITE");
m_database.setDatabaseName("device.db");
```

数据库成功打开后创建系统所需的数据表。

---

## 三、设备历史数据表设计

建立 `device_history` 表，用于保存设备运行过程中产生的历史监测数据。

表结构如下：

| 字段 | 类型 | 说明 |
|---|---|---|
| id | INTEGER | 历史数据记录编号，主键 |
| device_id | INTEGER | 设备编号 |
| temperature | REAL | 设备温度 |
| voltage | REAL | 设备电压 |
| online | INTEGER | 设备在线状态 |
| timestamp | DATETIME | 数据采集时间 |

数据模型：

```text
device_history
│
├── id
├── device_id
├── temperature
├── voltage
├── online
└── timestamp
```

其中 `device_id + timestamp` 构成历史数据查询中的主要检索条件。

---

## 四、实现设备数据插入

在 `DatabaseManager` 中增加 `insertDeviceData()` 接口。

设备产生新的监测数据后，将：

- 设备编号
- 温度
- 电压
- 在线状态
- 当前时间

写入 SQLite 数据库。

采用参数绑定方式执行 SQL：

```cpp
query.prepare(...);

query.bindValue(":device_id", deviceId);
query.bindValue(":temperature", data.temperature);
query.bindValue(":voltage", data.voltage);
query.bindValue(":online", data.isOnline);
query.bindValue(":timestamp", QDateTime::currentDateTime());

query.exec();
```

相比直接拼接 SQL 字符串，参数绑定能够使 SQL 执行更加规范，同时避免数据内容直接参与 SQL 字符串构造。

---

## 五、实现历史数据查询

增加：

```cpp
QList<DeviceHistory> queryDeviceHistory(int deviceId);
```

用于查询指定设备的全部历史监测数据。

查询结果按照时间升序排列：

```sql
WHERE device_id = :device_id
ORDER BY timestamp ASC
```

查询结果被转换为 `DeviceHistory` 对象，并通过 `QList<DeviceHistory>` 返回。

形成：

```text
SQLite
  ↓
QSqlQuery
  ↓
DeviceHistory
  ↓
QList<DeviceHistory>
```

为后续历史数据显示和曲线绘制提供数据基础。

---

## 六、实现时间范围查询

增加带时间范围的历史数据查询接口：

```cpp
QList<DeviceHistory> queryDeviceHistory(
    int deviceId,
    const QDateTime &startTime,
    const QDateTime &endTime);
```

可以根据：

- 设备编号
- 开始时间
- 结束时间

查询指定时间范围内的设备历史数据。

该接口主要为后续历史数据查询界面以及历史趋势曲线功能提供数据支持。

---

## 七、实现最新数据查询

增加：

```cpp
DeviceHistory queryLatestDeviceData(int deviceId);
```

通过：

```sql
ORDER BY timestamp DESC
LIMIT 1
```

获取指定设备最近的一条历史记录。

该接口可以用于：

- 获取设备最近一次状态
- 页面初始化时加载最新数据
- 后续设备状态恢复

---

## 八、实现历史数据删除

增加：

```cpp
bool deleteDeviceHistory(int deviceId);
```

用于删除指定设备的全部历史记录。

采用：

```sql
DELETE FROM device_history
WHERE device_id = :device_id
```

实现设备历史数据清理功能。

---

## 九、增加数据库索引

针对系统主要的历史数据查询方式：

```sql
WHERE device_id = ?
ORDER BY timestamp
```

建立联合索引：

```sql
CREATE INDEX IF NOT EXISTS
idx_device_history_device_time
ON device_history(device_id, timestamp);
```

使数据库能够针对设备编号和时间进行更有效的检索。

---

## 十、打通实时数据持久化链路

将 `DeviceManager` 的设备数据更新信号与 `DatabaseManager` 连接。

数据流变为：

```text
QTimer
   ↓
DeviceManager
   ↓
Device::updateData()
   ↓
DeviceData
   ↓
DeviceManager::deviceDataUpdated()
   ├──────────────→ MainWindow
   │                    ↓
   │                 实时数据显示
   │
   └──────────────→ DatabaseManager
                        ↓
                      SQLite
                        ↓
                 device_history
```

至此，系统中的设备模拟数据不再只存在于内存中，而是能够持续保存到本地数据库。

---

## 十一、事务设计分析

本阶段暂未对单条设备数据 INSERT 强制使用显式事务。

原因是当前一次设备数据更新对应一条独立的数据库写操作：

```text
一次数据采集
    ↓
一次 INSERT
```

不存在需要保证“多个 SQL 操作全部成功或全部失败”的复杂业务操作。

因此当前阶段不需要为了使用事务而使用事务。

后续如果出现以下业务：

```text
删除设备
    ↓
删除设备信息
    ↓
删除历史数据
    ↓
删除报警记录
    ↓
更新其他关联数据
```

或者进行大量历史数据批量写入时，再引入显式事务。

---

## 十二、今日测试

对数据库功能进行了基本验证：

### 1. 数据库连接

确认程序能够正常创建并打开：

```text
device.db
```

### 2. 数据表

确认：

```text
device_history
```

能够正常创建。

### 3. 数据写入

程序运行后，设备监测数据能够持续写入数据库。

### 4. 数据查询

验证指定设备历史数据和最新数据能够正常读取。

### 5. 时间范围查询

验证能够根据开始时间和结束时间查询对应历史记录。

### 6. 数据删除

验证指定设备历史数据能够正常删除。

---

## 十三、今日成果

Day3 完成后，项目已经具备基础的数据持久化能力：

```text
┌──────────────┐
│    Device    │
└──────┬───────┘
       │
       ↓
┌──────────────┐
│DeviceManager │
└──────┬───────┘
       │
       ↓
┌──────────────────┐
│ DatabaseManager  │
└────────┬─────────┘
         │
         ↓
┌──────────────────┐
│     SQLite       │
│ device_history   │
└──────────────────┘
```

系统目前已经能够完成：

> **设备数据产生 → 实时显示 → 数据持久化 → 历史数据查询**

为后续历史数据界面和数据可视化功能提供了完整的数据基础。

---

## 十四、下一步开发计划

下一阶段进入历史数据可视化功能：

1. 在设备详情界面增加历史数据查看功能
2. 显示历史温度、电压数据
3. 集成 Qt Charts
4. 绘制温度变化曲线
5. 绘制电压变化曲线
6. 增加时间范围筛选
7. 完成历史数据与实时监控之间的界面衔接

# Day4 开发日志

## 一、开发日期

2026-09-19

## 二、今日开发目标

在已有设备监控、设备详情和 SQLite 数据持久化功能的基础上，引入 Qt Charts，实现设备温度、电压数据的图形化展示，并逐步建立实时监控与历史数据展示的可视化基础。

今日重点完成：

1. 主窗口设备列表增加温度、电压趋势图。
2. 封装 `MiniChartWidget`，实现可复用的迷你折线图组件。
3. 将设备电压模拟数据调整至实际设备常见的 220V 附近。
4. 设备详情窗口增加温度、电压详细趋势图。
5. 调整设备详情窗口的数据展示方案，将其暂时定位为实时详细监控窗口。
6. 处理 Qt Charts 与 Qt Designer 布局冲突问题。

---

## 三、主要开发内容

### 3.1 主窗口增加实时趋势图

在原有设备监控表格的基础上，将表格列由原来的 4 列扩展为 6 列：

| 列 | 内容 |
|---|---|
| 1 | 设备ID |
| 2 | 状态 |
| 3 | 温度 |
| 4 | 电压 |
| 5 | 温度走势图 |
| 6 | 电压走势图 |

每台设备对应两个迷你折线图，用于显示最近一段时间内的温度和电压变化趋势。

为了避免频繁查询 SQLite，在主窗口中采用内存缓存保存最近的数据：

```cpp
QMap<int, QList<double>> m_temperatureHistory;
QMap<int, QList<double>> m_voltageHistory;
```

并设置最大保存点数：

```cpp
static constexpr int MaxHistoryPoints = 30;
```

由于当前设备数据每秒更新一次，因此每台设备的迷你图大约显示最近 30 秒的数据。

---

### 3.2 封装 MiniChartWidget

为了避免在 `MainWindow` 中重复编写 Qt Charts 代码，将迷你折线图封装为独立组件：

```cpp
enum class ChartType
{
    Temperature,
    Voltage
};
```

`MiniChartWidget` 根据图表类型分别负责：

- 创建 `QChart`
- 创建 `QLineSeries`
- 创建 X/Y 轴
- 设置坐标轴范围
- 接收外部数据
- 更新折线图

主窗口只需要调用：

```cpp
chart->setData(...);
```

即可完成图表更新。

这种方式降低了 `MainWindow` 与具体图表实现之间的耦合，也方便后续继续增加其他类型的数据图表。

---

### 3.3 调整电压模拟数据

之前模拟设备电压的数据范围较低，不符合当前设备监控系统的实际表现。

将电压模拟值调整为 220V 附近：

```cpp
m_data.voltage =
    220.0 +
    (QRandomGenerator::global()->generateDouble() - 0.5) * 10.0;
```

因此模拟电压大约处于：

```text
215V ~ 225V
```

同时将设备初始电压调整为：

```cpp
m_data.voltage = 220.0;
```

对应的电压趋势图 Y 轴调整为：

```cpp
m_voltageAxisY->setRange(210, 230);
```

使图表显示范围更加合理。

---

### 3.4 优化迷你图坐标轴显示

对 Qt Charts 的坐标轴进行了进一步调整。

X 轴使用 `QValueAxis` 时，默认数据类型为浮点数，因此显示的刻度可能带有小数。

通过：

```cpp
m_axisX->setLabelFormat("%.0f");
```

将其调整为整数显示。

同时考虑到主窗口表格中的空间有限，图表中的单位不再单独占用过多空间，而是通过图表标题表达，例如：

```text
温度 (°C)
电压 (V)
```

并对表格中的图表列使用 `Stretch`，使图表能够随窗口大小进行弹性调整。

---

## 四、设备详情窗口图表

在 `DeviceWidget` 中增加了更加详细的温度和电压趋势图。

与主窗口的迷你图不同，设备详情窗口使用：

- `QLineSeries`
- `QDateTimeAxis`
- `QValueAxis`
- `QChartView`

其中 X 轴使用实际时间：

```text
HH:mm:ss
```

因此可以直接观察设备数据随时间的变化。

详情窗口包含：

```text
设备当前状态
        ↓
当前温度
        ↓
当前电压
        ↓
温度实时趋势图
        ↓
电压实时趋势图
```

---

## 五、解决 Qt Designer 布局冲突

在将图表添加到 `DeviceWidget` 时出现了 Qt 警告：

```text
QLayout: Attempting to add QLayout "" to QWidget "temperatureChartWidget", which already has a layout

QLayout: Attempting to add QLayout "" to QWidget "voltageChartWidget", which already has a layout
```

经分析发现：

`temperatureChartWidget` 和 `voltageChartWidget` 已经在 Qt Designer 中设置了布局。

因此代码中再次执行：

```cpp
new QVBoxLayout(ui->temperatureChartWidget);
```

属于重复创建布局。

最终改为直接使用 Designer 已经创建的布局：

```cpp
ui->temperatureChartWidget->layout()->addWidget(
    m_temperatureChartView
);

ui->voltageChartWidget->layout()->addWidget(
    m_voltageChartView
);
```

从而消除了布局冲突。

---

## 六、实时数据与历史数据的职责划分

今日开发过程中进一步明确了实时监控与历史数据的职责。

当前系统采用：

```text
Device
  │
  │ dataUpdated
  ↓
DeviceManager
  │
  ├──────────────→ MainWindow
  │                   └→ 实时迷你图
  │
  ├──────────────→ DeviceWidget
  │                   └→ 实时详细图
  │
  └──────────────→ DatabaseManager
                      └→ SQLite 持久化
```

其中：

### 实时显示

直接使用：

```cpp
Device::dataUpdated
```

传递的数据进行图表更新。

### 数据持久化

由：

```cpp
DatabaseManager
```

将设备数据写入 SQLite。

### 历史数据

SQLite 中已经保存完整历史数据，但当前阶段暂不直接用于实时详细图。

原因是如果每次刷新图表都从数据库读取历史数据，会让实时显示逻辑和历史查询逻辑产生较强耦合。

因此当前先完成：

> 实时详细监控窗口

后续再独立实现：

> 历史数据查询与历史趋势分析

---

## 七、当前 DeviceWidget 数据流

当前设备详情窗口的实时数据流程为：

```text
Device::updateData()
        │
        ↓
emit dataUpdated()
        │
        ├──────────────→ updateWidget()
        │                      │
        │                      ├→ 更新状态
        │                      ├→ 更新温度
        │                      ├→ 更新电压
        │                      │
        │                      └→ updateCharts()
        │
        └──────────────→ DatabaseManager
                               │
                               ↓
                            SQLite
```

图表只保留最近一定数量的数据点，从而避免程序运行时间过长后图表数据无限增长。

---

## 八、今日遇到的问题

### 问题 1：启动设备后图表崩溃

初版 Qt Charts 实现中，图表坐标轴及数据更新逻辑存在初始化问题。

经过调整，将坐标轴创建、添加以及 Series 与 Axis 的绑定关系明确化，最终解决了启动设备后图表更新导致的崩溃问题。

### 问题 2：Qt Designer 布局重复创建

发现 `temperatureChartWidget` 和 `voltageChartWidget` 已经存在布局，代码中再次创建布局产生 Qt 警告。

最终改为直接使用 Designer 中已有的布局。

### 问题 3：设备详情图表无法实时更新

最初详情窗口只在打开时执行一次：

```cpp
loadHistory();
```

因此只能读取打开窗口时已经存在的 SQLite 历史数据，之后产生的新数据不会自动进入图表。

经过分析，将设备详情窗口当前阶段调整为实时监控模式，通过 `Device::dataUpdated` 直接更新图表。

历史数据查询功能暂时保留在数据层，后续单独设计历史数据查看功能。

---

## 九、今日开发结果

今日完成 Qt Charts 第一阶段集成，当前系统已经具备：

- 设备实时温度显示
- 设备实时电压显示
- 主窗口温度迷你趋势图
- 主窗口电压迷你趋势图
- 设备详情温度趋势图
- 设备详情电压趋势图
- 实时数据与 SQLite 持久化并行工作
- 220V 附近的电压模拟数据
- 图表组件初步封装
- 图表窗口弹性布局

当前系统已经从单纯的：

> “实时数值监控”

进一步发展为：

> “实时数值 + 实时趋势图 + 数据持久化”

---

## 十、下一步计划

下一阶段可以在当前实时监控功能稳定后继续完善：

1. 完善设备详情实时图表的坐标轴动态范围。
2. 增加实时图表最大数据点限制。
3. 增加历史数据查询功能。
4. 增加时间范围选择，例如：
   - 最近 1 分钟
   - 最近 10 分钟
   - 最近 1 小时
   - 自定义时间范围
5. 将 SQLite 历史数据加载到独立的历史趋势界面。
6. 后续再进入串口通信 / Modbus 等真实设备通信模块。

当前阶段暂不引入串口和多线程，优先保证现有数据链路和图表功能稳定。

# Day5 开发日志——历史数据查询与趋势分析

**日期：2026-09-22**

## 一、今日开发目标

在前期完成 SQLite 数据持久化和 Qt Charts 实时数据展示的基础上，为设备详情页面增加历史数据查询与历史趋势分析功能，使系统能够同时支持：

- 实时设备数据监控
- 设备历史数据查询
- 历史温度趋势分析
- 历史电压趋势分析
- 不同时间范围的数据筛选

进一步完善设备监控上位机的数据分析能力。

---

## 二、今日主要开发内容

### 1. 完善设备详情页面历史数据区域

在 `DeviceWidget` 中增加历史数据展示区域，与原有实时数据区域进行区分。

设备详情页面目前形成两个相对独立的数据展示区域：

```text
设备详情
├── 实时数据
│   ├── 实时温度
│   └── 实时电压
│
└── 历史数据
    ├── 温度历史趋势
    ├── 电压历史趋势
    └── 时间范围查询
```

实时图表继续负责当前设备运行状态的动态监控，历史图表负责 SQLite 中已保存数据的查询与分析。

---

### 2. 实现历史温度与电压图表

使用 Qt Charts 分别创建历史温度和历史电压图表。

历史图表使用：

- `QLineSeries`：保存历史数据曲线
- `QDateTimeAxis`：显示时间轴
- `QValueAxis`：显示温度、电压数值
- `QChartView`：嵌入 Qt Designer 中的历史数据区域

温度和电压分别使用独立的 Series 与坐标轴，避免实时数据和历史数据之间相互影响。

---

### 3. 实现历史数据时间范围查询

为历史数据区域增加时间范围选择功能，目前支持：

```text
最近1分钟
最近10分钟
最近1小时
最近24小时
```

点击查询按钮后，根据当前选择的时间范围计算：

```cpp
endTime = QDateTime::currentDateTime();
startTime = endTime - 对应时间范围;
```

随后调用 `DatabaseManager` 的时间范围查询接口，从 SQLite 中获取对应设备的历史数据。

---

### 4. 完善 SQLite 时间数据处理

之前数据库中的时间字段采用：

```text
yyyy-MM-dd HH:mm:ss
```

格式进行保存。

在读取历史数据时，进一步明确使用对应格式将数据库中的字符串转换为 `QDateTime`：

```cpp
history.timestamp =
    QDateTime::fromString(
        query.value("timestamp").toString(),
        "yyyy-MM-dd HH:mm:ss"
    );
```

解决历史图表 `QDateTimeAxis` 时间数据解析问题，使历史数据能够正确映射到时间轴。

---

### 5. 增加历史数据采样机制

考虑到长时间范围内可能存在大量历史数据，例如：

```text
1秒/条 × 24小时 ≈ 86400条
```

如果直接将全部数据加载到 Qt Charts 中，会造成不必要的绘制和数据处理压力。

因此增加：

```cpp
sampleHistory(history, 300);
```

将历史数据限制在最多约 300 个绘图点。

采用等间隔采样方式，在尽可能保留整体趋势的情况下减少图表绘制数据量。

数据处理流程变为：

```text
SQLite
  ↓
时间范围查询
  ↓
获取原始历史数据
  ↓
sampleHistory()
  ↓
最多300个数据点
  ↓
QLineSeries
  ↓
历史趋势图
```

---

### 6. 实现历史图表动态坐标范围

根据当前查询到的历史数据计算温度和电压的最小值、最大值，并为坐标轴增加一定边距。

温度和电压的 Y 轴不再完全固定，而是根据当前查询数据动态调整，使历史趋势更加容易观察。

X 轴则根据查询结果的首尾时间设置范围：

```text
第一条历史数据
      ↓
    X轴起点
      ...
    X轴终点
      ↓
最后一条历史数据
```

---

### 7. 修正历史图表布局问题

开发过程中发现历史图表虽然设置了与实时图相同的固定高度，但实际显示尺寸明显小于实时图。

经排查发现主要原因是历史图表所在 Layout 存在额外的边距和间距。

对历史图表 Layout 进行调整：

```cpp
setContentsMargins(0, 0, 0, 0);
setSpacing(0);
```

同时清理历史图表区域中原有的占位 `QLabel`。

调整后，`QChartView` 能够充分利用历史图表区域的高度，历史图与实时图的显示尺寸基本保持一致。

---

## 三、当前设备详情页结构

经过今日开发，目前设备详情页的数据展示结构为：

```text
DeviceWidget
│
├── 设备基本信息
│
├── 实时数据
│   ├── 实时温度曲线
│   └── 实时电压曲线
│
├── 历史数据
│   ├── 温度历史趋势
│   ├── 电压历史趋势
│   └── 时间范围选择 + 查询
│
└── 设备控制
    ├── 启动
    └── 停止
```

设备数据链路进一步完善为：

```text
Device
   │
   ↓
DeviceManager
   │
   ├──────────────→ MainWindow
   │                    ↓
   │              实时监控/趋势
   │
   ├──────────────→ DeviceWidget
   │                    ↓
   │              实时详细数据
   │
   └──────────────→ DatabaseManager
                        ↓
                     SQLite
                        ↓
                  历史数据查询
                        ↓
                  DeviceWidget
                        ↓
                历史趋势分析
```

---

## 四、今日遇到的问题与解决

### 问题1：历史图表显示尺寸明显小于实时图表

**原因：**

历史图表 Layout 存在额外的 `ContentsMargins` 和间距，并且保留了 Designer 中的占位控件。

**解决：**

清除 Layout 边距与间距，并删除不再使用的占位 `QLabel`。

---

### 问题2：历史数据时间轴显示异常

**原因：**

SQLite 中保存的时间本质上是格式化后的字符串，而读取时直接调用 `toDateTime()` 存在解析不明确的问题。

**解决：**

使用：

```cpp
QDateTime::fromString(
    query.value("timestamp").toString(),
    "yyyy-MM-dd HH:mm:ss"
);
```

显式解析时间字符串。

---

### 问题3：长时间历史数据可能产生大量绘图点

**原因：**

实时数据以约 1 秒一个数据点持续写入数据库，24 小时可能产生数万甚至数十万个数据点。

**解决：**

增加 `sampleHistory()`，将历史趋势图绘制数据控制在最多约 300 个点。

---

## 五、今日完成情况

今日完成了设备监控系统从“实时监控”向“历史数据分析”的功能扩展。

目前系统已经能够实现：

- SQLite 历史数据持久化
- 按设备查询历史数据
- 按时间范围查询历史数据
- 最近1分钟数据查询
- 最近10分钟数据查询
- 最近1小时数据查询
- 最近24小时数据查询
- 历史数据采样
- 历史温度趋势图
- 历史电压趋势图
- 时间轴显示
- 动态 Y 轴范围
- 实时数据与历史数据分离展示

至此，设备详情页面已经具备较完整的实时监控与历史趋势分析能力。

---

## 六、下一步开发计划

下一阶段准备进一步完善设备监控系统的运行状态管理，包括：

1. 完善设备在线/离线状态显示；
2. 增加温度、电压等参数的异常判断；
3. 设计设备报警机制；
4. 增加报警信息展示；
5. 根据需要增加报警历史记录。

在此基础上，再逐步进入串口通信、Modbus 等真实设备通信功能的开发。

# Day6 开发日志——设备状态与报警管理模块

## 一、开发日期

2026 年 9 月

## 二、开发目标

在前期已经完成设备模拟数据、设备详情显示、SQLite 数据持久化、实时趋势图以及历史数据查询等功能的基础上，本阶段进一步完善设备监控系统的异常处理能力。

前期系统主要解决了“设备数据如何产生、显示和保存”的问题，但对于设备运行过程中出现的异常情况，还缺少统一的检测和管理机制。

因此，本阶段的主要目标是：

1. 建立统一的报警数据结构；
2. 实现设备温度、电压和在线状态的异常检测；
3. 实现报警去重机制，避免同一异常持续发生时重复产生报警；
4. 实现报警恢复检测；
5. 在主界面显示报警事件和当前未恢复报警；
6. 将报警事件保存至 SQLite 数据库；
7. 实现程序重新启动后当前报警状态的恢复。

---

## 三、报警模块设计

### 3.1 报警类型设计

在 `Core` 模块中新增 `Alarm.h`，定义系统支持的报警类型：

```cpp
enum class AlarmType
{
    TemperatureHigh,
    TemperatureLow,
    VoltageHigh,
    VoltageLow,
    DeviceOffline
};
```

当前系统主要检测三类设备状态：

- 温度异常；
- 电压异常；
- 设备离线。

其中温度和电压分别进一步划分为过高和过低两种情况。

---

### 3.2 报警信息结构

定义 `AlarmInfo` 作为系统中的统一报警数据对象：

```cpp
struct AlarmInfo
{
    int deviceId = -1;
    AlarmType type;
    QString message;
    QDateTime timestamp;
    bool recovered = false;
};
```

其中：

| 字段 | 含义 |
|---|---|
| `deviceId` | 发生报警的设备 ID |
| `type` | 报警类型 |
| `message` | 报警具体信息 |
| `timestamp` | 报警发生或恢复时间 |
| `recovered` | 是否为恢复事件 |

`recovered = false` 表示报警发生，`recovered = true` 表示之前的报警已经恢复。

---

## 四、AlarmManager 报警管理模块

### 4.1 模块职责

新增 `AlarmManager`，负责从设备数据中判断当前设备是否处于异常状态。

其职责与其他模块保持独立：

```text
Device
   ↓
DeviceManager
   ↓
AlarmManager
   ↓
AlarmInfo
   ├──→ MainWindow
   └──→ DatabaseManager
```

其中：

- `Device`：保存设备自身数据；
- `DeviceManager`：统一管理多个设备；
- `AlarmManager`：负责报警检测和报警状态管理；
- `MainWindow`：负责报警信息显示；
- `DatabaseManager`：负责报警数据持久化。

这种设计避免了将报警判断逻辑直接写入界面代码。

---

### 4.2 报警阈值

当前系统采用以下模拟设备运行范围：

| 监测项目 | 正常范围 | 报警条件 |
|---|---|---|
| 温度 | 15～50 ℃ | `<15 ℃` 或 `>50 ℃` |
| 电压 | 210～230 V | `<210 V` 或 `>230 V` |
| 在线状态 | 在线 | `isOnline == false` |

报警检测统一通过 `checkAlarm()` 完成。

例如：

```cpp
checkAlarm(
    deviceId,
    AlarmType::TemperatureHigh,
    data.temperature > 50,
    "温度过高"
);
```

这种统一处理方式减少了不同报警类型之间的重复代码。

---

## 五、报警去重与恢复机制

设备数据每秒更新一次，如果某个设备持续处于异常状态，不能每秒产生一条新的报警记录。

因此 `AlarmManager` 使用：

```cpp
QMap<int, QList<AlarmType>> m_activeAlarms;
```

保存当前已经处于激活状态的报警。

报警处理逻辑为：

```text
检测到异常
   ↓
是否已经存在该报警？
   ├── 是 → 不重复产生报警
   └── 否 → 创建新的 AlarmInfo
                ↓
          加入活动报警
                ↓
          发出 alarmTriggered
```

当设备恢复正常时：

```text
检测到恢复
   ↓
之前是否存在该报警？
   ├── 否 → 不处理
   └── 是 → 创建恢复事件
                ↓
          清除活动报警
                ↓
          发出 alarmTriggered
```

因此可以正确处理：

```text
报警 → 持续异常 → 恢复 → 再次报警
```

同一种异常在持续期间不会被重复记录，而恢复后再次发生时可以重新产生报警。

---

## 六、DeviceManager 集成 AlarmManager

在 `DeviceManager` 中增加 `AlarmManager`：

```cpp
AlarmManager *m_alarmManager;
```

设备更新完成后进行报警检查：

```cpp
m_alarmManager->checkDeviceData(
    device->id(),
    device->data()
);
```

同时将 `AlarmManager` 的报警信号转发出去：

```cpp
void alarmTriggered(const AlarmInfo &alarm);
```

这样 `MainWindow` 无需直接依赖 `AlarmManager` 的内部实现，只需要监听 `DeviceManager` 的报警信号。

---

## 七、MainWindow 报警显示

主界面新增两个报警表格。

### 7.1 报警事件表

`alarmTable` 用于保存所有报警事件，包括报警发生和报警恢复。

表格字段：

```text
设备ID | 类型 | 信息 | 时间
```

例如：

```text
1 | 报警 | 温度过高 | 15:21:03
1 | 恢复 | 温度过高，已恢复正常 | 15:25:17
```

该表用于记录报警事件的完整过程。

---

### 7.2 当前报警表

`currentAlarmTable` 只显示当前尚未恢复的报警。

表格字段：

```text
设备ID | 报警类型 | 报警信息 | 发生时间
```

例如：

```text
1 | 温度 | 温度过高 | 15:21:03
2 | 电压 | 电压过低 | 15:23:11
```

当报警恢复时，对应记录会从当前报警表中删除。

因此两个表承担不同职责：

```text
alarmTable
    ↓
所有历史报警事件

currentAlarmTable
    ↓
当前仍然存在的报警
```

---

## 八、Qt::UserRole 保存报警类型

为了在表格中显示可读的报警类型，同时保留程序内部使用的 `AlarmType`，在 `QTableWidgetItem` 中使用 `Qt::UserRole` 保存枚举值。

写入：

```cpp
typeItem->setData(
    Qt::UserRole,
    static_cast<int>(alarm.type)
);
```

读取：

```cpp
AlarmType currentType =
    static_cast<AlarmType>(
        item->data(Qt::UserRole).toInt()
    );
```

这样可以将：

```text
程序内部：
TemperatureHigh
```

和：

```text
界面显示：
温度
```

分离。

同时通过 `deviceId + AlarmType` 判断当前报警是否已经存在。

---

## 九、SQLite 报警历史持久化

### 9.1 创建报警历史表

在 `DatabaseManager::createTables()` 中增加：

```sql
CREATE TABLE IF NOT EXISTS alarm_history
(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    device_id INTEGER NOT NULL,
    alarm_type INTEGER NOT NULL,
    message TEXT NOT NULL,
    recovered INTEGER NOT NULL,
    timestamp DATETIME NOT NULL
)
```

数据库结构如下：

| 字段 | 类型 | 作用 |
|---|---|---|
| `id` | INTEGER | 报警记录 ID |
| `device_id` | INTEGER | 设备 ID |
| `alarm_type` | INTEGER | 报警类型 |
| `message` | TEXT | 报警信息 |
| `recovered` | INTEGER | 是否为恢复事件 |
| `timestamp` | DATETIME | 时间 |

由于 SQLite 不直接保存 C++ `enum class`，因此将 `AlarmType` 转换为整数进行存储。

同时：

```text
recovered = 0
```

表示报警发生；

```text
recovered = 1
```

表示报警恢复。

---

### 9.2 报警索引

为报警历史数据增加：

```sql
CREATE INDEX IF NOT EXISTS
idx_alarm_history_device_time
ON alarm_history(device_id, timestamp)
```

用于提高按照设备和时间查询报警记录时的效率。

---

## 十、报警记录写入数据库

在 `DatabaseManager` 中增加：

```cpp
void insertAlarm(const AlarmInfo &alarm);
```

报警发生或恢复时，将 `AlarmInfo` 写入 `alarm_history`。

数据流为：

```text
AlarmManager
     ↓
AlarmInfo
     ↓
MainWindow::handleAlarm()
     ├── 更新界面
     └── DatabaseManager::insertAlarm()
                    ↓
             alarm_history
```

这样报警信息不会因为程序关闭而丢失。

---

## 十一、报警历史查询

增加：

```cpp
QList<AlarmInfo> queryAlarmHistory();
```

程序启动后从 `alarm_history` 中读取历史报警记录，并加载到 `alarmTable`。

数据库中的字符串时间通过：

```cpp
QDateTime::fromString(
    query.value("timestamp").toString(),
    "yyyy-MM-dd HH:mm:ss"
);
```

重新转换为 `QDateTime`。

---

## 十二、程序启动后的当前报警恢复

仅仅加载报警历史还不够。

例如数据库中存在：

```text
设备1 温度报警
设备1 温度恢复
设备2 电压报警
```

程序重新启动后，设备1不应该被认为仍然处于报警状态，而设备2应该恢复为当前报警。

因此增加：

```cpp
QList<AlarmInfo> queryActiveAlarms();
```

查询每一个：

```text
设备ID + 报警类型
```

的最后一条报警记录。

如果最后一条记录：

```text
recovered = 0
```

则说明该报警目前仍未恢复，应重新加入 `currentAlarmTable`。

由此实现：

```text
程序运行
   ↓
产生报警
   ↓
保存 SQLite
   ↓
关闭程序
   ↓
重新启动
   ↓
读取 alarm_history
   ↓
恢复历史报警
   ↓
恢复当前未解决报警
```

---

## 十三、报警统计

主界面增加报警数量统计，用于显示：

```text
报警事件：XX
当前报警：XX
```

通过：

```cpp
void MainWindow::updateAlarmStatistics()
```

统一根据两个表格的行数更新统计信息。

报警发生时：

```text
报警事件 +1
当前报警 +1
```

报警恢复时：

```text
报警事件 +1
当前报警 -1
```

程序启动加载数据库后也会重新统计。

---

## 十四、测试情况

本阶段对报警模块进行了以下测试。

### 1. 持续报警测试

设备持续超过温度阈值时，只产生一次报警。

结果：

```text
报警 → 正常
```

不会产生大量重复记录。

### 2. 报警恢复测试

设备从异常状态恢复正常后：

```text
报警事件表：增加一条恢复记录
当前报警表：删除对应报警
```

### 3. 再次报警测试

设备恢复后再次进入异常状态，可以重新产生报警。

测试流程：

```text
报警
 ↓
恢复
 ↓
再次报警
```

能够正常工作。

### 4. 多报警同时存在

同一设备可以同时存在不同类型的报警。

例如：

```text
设备1 | 温度 | 温度过高
设备1 | 电压 | 电压过高
```

两种报警互不影响。

### 5. 程序重启测试

程序关闭后重新启动，从 SQLite 中读取报警历史，并恢复当前未解决报警。

---

## 十五、本阶段遇到的问题

### 15.1 持续异常导致报警重复触发

最初如果每次设备数据更新都直接发送报警信号，那么一个持续异常可能每秒产生一条报警。

通过 `m_activeAlarms` 保存当前活动报警状态后解决。

---

### 15.2 报警类型的显示和程序判断存在冲突

界面需要显示：

```text
温度
```

但程序判断需要区分：

```text
TemperatureHigh
TemperatureLow
```

因此使用 `Qt::UserRole` 保存实际的 `AlarmType`，显示文本和内部数据分离。

---

### 15.3 程序重启后无法直接判断当前报警

历史报警记录和当前报警状态不是同一个概念。

通过查询每一个“设备 + 报警类型”的最后一条记录解决了这一问题。

---

## 十六、本阶段完成情况

Day6 完成了设备监控系统基础报警管理模块，实现：

- [x] `AlarmInfo` 报警数据结构
- [x] 多种报警类型定义
- [x] 温度异常检测
- [x] 电压异常检测
- [x] 设备离线检测
- [x] 报警去重
- [x] 报警恢复
- [x] 多报警并存
- [x] 报警事件记录
- [x] 当前报警管理
- [x] SQLite 报警历史持久化
- [x] 报警历史查询
- [x] 当前报警状态恢复
- [x] 报警数量统计
- [x] 报警数据库索引

至此，系统已经从单纯的设备数据监控进一步扩展为具备基础异常检测和报警管理能力的设备监控上位机。

---

## 十七、下一阶段计划

下一阶段进入**串口通信模块开发**。

当前系统中的设备数据仍然主要由 `Device` 内部模拟生成，下一阶段将逐步建立真实设备通信链路：

```text
真实设备
    ↓
串口通信
    ↓
SerialPort
    ↓
数据解析
    ↓
DeviceManager
    ↓
Device
    ↓
AlarmManager
    ↓
MainWindow / DatabaseManager
```

在进入真实串口通信之前，需要适当调整 `Device` 的数据更新方式，使设备对象既能够继续支持当前的模拟数据，又能够接收来自通信模块的真实设备数据。

---

