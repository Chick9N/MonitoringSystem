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
# Day7 开发日志：串口通信与设备数据接入

## 一、今日开发目标

本阶段的目标是将设备监控系统从“程序内部模拟设备数据”逐步扩展为能够接收外部真实设备数据的上位机系统。

核心数据链路设计为：

```text
真实设备
   ↓
串口通信
   ↓
SerialPort
   ↓ 原始字节
ProtocolParser
   ↓ DeviceData
DeviceManager
   ↓
Device
   ↓
监控界面 / 报警 / 数据库 / 历史曲线
```

---

## 二、今日完成内容

### 1. 完善 Device 数据更新机制

在 `Device` 中增加 `setData()`：

```cpp
void Device::setData(const DeviceData &data)
{
    m_data = data;
    emit dataUpdated(m_data);
}
```

这样设备数据不再只能由 `Device::updateData()` 内部模拟产生，也可以由外部通信模块向设备对象写入。

原有的模拟数据生成机制暂时保留，用于没有真实硬件时进行系统测试。

---

### 2. 完成 SerialPort 串口通信模块

新增 `SerialPort`，对 Qt 的 `QSerialPort` 进行封装。

主要功能：

- 打开串口
- 关闭串口
- 判断串口状态
- 发送数据
- 接收数据
- 获取可用串口
- 处理串口错误
- 对接收到的数据进行缓存

核心设计：

```text
QSerialPort
    ↓
SerialPort
    ↓
dataReceived(QByteArray)
```

上层不直接操作 `QSerialPort`，从而避免通信细节进入业务层。

---

### 3. 完成串口数据缓冲与拆包

针对串口通信可能出现的：

- 半包
- 粘包
- 无效数据

增加接收缓冲区 `m_buffer`，按照固定协议帧进行处理。

当前测试协议长度为 7 字节：

```text
AA 设备ID 温度 电压高字节 电压低字节 在线状态 55
```

其中：

- `AA`：帧头
- `设备ID`：设备编号
- `温度`：温度值
- `电压高字节 + 电压低字节`：电压
- `在线状态`：设备在线状态
- `55`：帧尾

---

### 4. 完成 ProtocolParser 协议解析模块

新增 `ProtocolParser`，负责将串口收到的原始字节转换为业务层可以直接使用的 `DeviceData`。

例如：

```text
AA 01 20 00 DC 01 55
```

解析结果：

```text
设备ID：1
温度：32 ℃
电压：220 V
在线：true
```

这样 `DeviceManager` 不需要了解具体字节协议，只接收已经解析完成的设备数据。

---

### 5. 将串口通信接入 DeviceManager

当前通信架构：

```text
MainWindow
    ↓
DeviceManager
    ↓
SerialPort
    ↓
QSerialPort
```

`DeviceManager` 持有 `SerialPort`，并负责：

1. 接收串口原始数据
2. 调用 `ProtocolParser`
3. 得到 `deviceId + DeviceData`
4. 找到对应 Device
5. 更新 Device
6. 触发已有的监控、报警等逻辑

新增：

```cpp
void DeviceManager::updateDeviceData(
    int deviceId,
    const DeviceData &data);
```

---

### 6. 增加数据来源控制

增加：

```cpp
enum class DataSource
{
    Simulation,
    Serial
};
```

通过数据来源控制避免模拟数据与串口数据互相覆盖。

模拟模式下：

```text
Device::updateData()
```

继续产生模拟数据。

串口模式下：

```text
SerialPort → ProtocolParser → DeviceManager
```

负责提供设备数据。

---

### 7. 完成串口通信链路测试

在没有真实串口设备的情况下，增加了临时测试接口 `simulateReceive()`，用于模拟串口收到的数据。

已经验证：

#### 完整数据帧

能够正确解析并更新设备数据。

#### 半包

例如先接收：

```text
AA 01 20
```

随后接收：

```text
00 DC 01 55
```

最终能够正确拼接并解析。

#### 粘包

一次接收多个连续数据帧：

```text
AA 01 20 00 DC 01 55
AA 02 22 00 DD 01 55
```

能够分别解析为设备 1 和设备 2 的数据。

#### 完整链路

日志已经确认：

```text
收到设备数据
    ↓
DeviceManager 转发数据
    ↓
MainWindow 收到数据
```

因此当前：

**SerialPort → ProtocolParser → DeviceManager → Device → MainWindow**

这一条核心链路已经打通。

---

## 三、当前架构状态

目前系统已经从单纯的模拟监控系统进一步发展为：

```text
                 ┌──────────────┐
                 │   真实设备    │
                 └──────┬───────┘
                        │
                    串口数据
                        ↓
                 ┌──────────────┐
                 │  SerialPort  │
                 └──────┬───────┘
                        │
                     原始帧
                        ↓
              ┌──────────────────┐
              │ ProtocolParser   │
              └────────┬─────────┘
                       │
                  DeviceData
                       ↓
              ┌──────────────────┐
              │  DeviceManager   │
              └────────┬─────────┘
                       │
              ┌────────┼─────────┐
              ↓        ↓         ↓
           Device   AlarmManager  UI
              │
              ↓
       DatabaseManager
```

这个架构保持了通信层、协议层、业务层和 UI 层之间的职责分离。

---

## 四、今天暂未完成的内容

今天主要完成的是**串口通信底层链路**，以下内容暂时没有继续实现：

### 1. 串口配置 UI

暂时没有在 MainWindow 中放置大量串口配置控件。

最终决定采用：

```text
MainWindow
    ↓
[串口设备管理]
    ↓
SerialConfigWindow
```

MainWindow 只提供一个进入串口设备管理的入口。

### 2. 新设备自动注册

目前 `DeviceManager::updateDeviceData()` 如果收到一个不存在的设备 ID，暂时无法将其自动加入系统。

例如：

```text
已有：
Device 1
Device 2
Device 3

串口收到：
Device 4
```

下一阶段需要实现：

```text
发现未知 Device ID
        ↓
创建 Device
        ↓
加入 DeviceManager
        ↓
进入正常监控流程
```

这将是“通过串口接入新设备”真正完成的关键一步。

### 3. 真实串口硬件测试

当前电脑没有可用的真实串口设备，因此使用模拟接收接口完成协议和数据链路测试。

后续有真实设备或虚拟串口环境后，再进行实际通信测试。

---

# Day8 开发计划：串口设备管理与新设备接入

## 一、第一阶段：创建串口设备管理窗口

新增：

```text
SerialConfigWindow
```

MainWindow 不直接管理串口参数，只保留：

```text
[串口设备管理]
```

按钮。

点击后打开串口设备管理窗口。

---

## 二、第二阶段：完善串口配置

在 `SerialConfigWindow` 中提供：

- 串口选择
- 波特率
- 数据位
- 校验位
- 停止位
- 打开串口
- 关闭串口
- 刷新串口
- 当前串口状态

推荐的波特率选项：

```text
9600
19200
38400
57600
115200
```

窗口通过 `DeviceManager` 操作串口，而不是直接操作 `QSerialPort`。

保持：

```text
SerialConfigWindow
        ↓
DeviceManager
        ↓
SerialPort
        ↓
QSerialPort
```

---

## 三、第三阶段：实现未知设备自动注册

修改：

```cpp
DeviceManager::updateDeviceData()
```

目标逻辑：

```text
收到设备数据
      ↓
根据 deviceId 查找 Device
      ↓
 ┌────┴────┐
存在       不存在
 ↓           ↓
更新       创建 Device
             ↓
          addDevice()
             ↓
           更新数据
```

例如串口第一次收到：

```text
AA 04 20 00 DC 01 55
```

系统自动创建：

```text
Device 4
```

并将其加入设备管理器。

之后设备 4 就可以自动进入：

- 实时监控
- 温度/电压曲线
- 报警检测
- 历史数据记录
- 设备详情窗口

---

## 四、第四阶段：验证完整的新设备接入流程

最终测试目标：

```text
模拟/真实设备
      ↓
串口
      ↓
SerialPort
      ↓
ProtocolParser
      ↓
发现 Device 4
      ↓
自动创建 Device 4
      ↓
加入 DeviceManager
      ↓
MainWindow 出现 Device 4
      ↓
实时数据持续更新
      ↓
报警与历史记录正常工作
```

完成这一流程后，“串口接入新设备”这一阶段的核心功能就基本成立。

---

> 今日日志重点记录“通信链路打通”，明日重点转向“设备接入管理”。UI 美化仍然不在当前阶段进行，统一放到功能开发基本完成后处理。

可以。下面直接按 **`.md` 纯文本格式** 放进代码框，你复制后保存为 `Day9_开发日志.md` 即可。

```markdown
# Day 9 开发日志——串口配置窗口完善

## 一、今日开发内容

今日主要完成设备监控上位机的**串口配置窗口**开发与问题排查，为后续接入真实串口设备做好准备。

本日工作重点从原有的串口通信底层实现，进一步扩展到上位机端的串口参数配置与串口状态管理。

---

## 二、串口配置窗口功能完善

### 1. 串口列表刷新

完善 `SerialConfigWindow` 中的串口刷新功能。

主要实现：

- 获取当前系统可用串口列表；
- 将可用串口添加至串口下拉框；
- 支持点击“刷新串口”按钮重新扫描；
- 当前没有可用串口时给出相应提示。

核心调用关系：

```text
SerialConfigWindow
        ↓
DeviceManager::availableSerialPorts()
        ↓
SerialPort::availablePorts()
        ↓
QSerialPortInfo::availablePorts()
```

通过该结构将 Qt 底层串口检测能力封装到 `DeviceManager`，由配置窗口负责显示。

---

### 2. 串口参数配置

完善串口参数配置界面，为用户提供常用串口通信参数选择。

目前包括：

- 波特率
  - 9600
  - 19200
  - 38400
  - 57600
  - 115200

- 数据位
  - 5
  - 6
  - 7
  - 8

- 校验位
  - 无校验
  - 奇校验
  - 偶校验

- 停止位
  - 1
  - 2

默认配置：

```text
波特率：9600
数据位：8
校验位：无
停止位：1
```

下拉框中的配置参数通过 `itemData` 保存对应的 `QSerialPort` 枚举值，打开串口时直接读取并传递给串口管理模块。

---

## 三、串口打开与关闭功能

### 1. 打开串口

完善 `on_openSerialButton_clicked()` 槽函数。

打开流程：

```text
选择串口
    ↓
读取串口参数
    ↓
DeviceManager::openSerialPort()
    ↓
SerialPort
    ↓
QSerialPort::open()
```

打开前首先检查是否选择了有效串口。

如果没有选择串口，则提示：

```text
请先选择串口
```

打开成功后：

- 将数据源切换为 `DataSource::Serial`
- 更新串口状态显示
- 更新相关按钮及参数控件状态

打开失败时，通过消息框向用户反馈错误信息。

---

### 2. 关闭串口

完善 `on_closeSerialButton_clicked()` 槽函数。

关闭流程：

```text
点击关闭串口
    ↓
DeviceManager::closeSerialPort()
    ↓
SerialPort::close()
    ↓
更新串口状态
```

关闭后重新更新界面状态，使串口配置窗口与底层实际连接状态保持一致。

---

## 四、串口状态管理

增加并完善串口状态显示逻辑。

通过：

```cpp
DeviceManager::isSerialPortOpen()
```

判断当前串口是否处于打开状态。

界面根据状态进行相应显示：

```text
串口已连接
串口未连接
```

同时根据连接状态控制：

- 打开串口按钮
- 关闭串口按钮
- 串口参数控件

的启用状态。

---

## 五、DeviceManager 与配置窗口的职责划分

进一步明确串口相关模块之间的职责。

当前结构：

```text
MainWindow
    │
    └── SerialConfigWindow
            │
            └── DeviceManager
                    │
                    └── SerialPort
                            │
                            └── QSerialPort
```

职责划分：

### MainWindow

负责：

- 提供进入串口设备管理功能的入口；
- 不直接处理串口参数。

### SerialConfigWindow

负责：

- 串口列表显示；
- 串口参数选择；
- 打开、关闭串口操作；
- 串口连接状态显示。

### DeviceManager

负责：

- 对上层提供统一的串口管理接口；
- 管理串口数据源；
- 接收并解析串口数据；
- 将解析后的设备数据更新到对应设备。

### SerialPort

负责：

- 封装 `QSerialPort`；
- 串口打开与关闭；
- 数据发送；
- 数据接收；
- 接收缓冲；
- 串口底层错误处理。

这种结构避免 `SerialConfigWindow` 直接操作 `QSerialPort`，降低界面层与底层通信模块之间的耦合。

---

## 六、信号与槽重复连接问题排查

今日开发过程中发现点击串口相关按钮时，提示框会出现两次。

经过检查发现，按钮同时存在两种信号槽连接方式：

### 方式一：Qt Designer 自动连接

例如：

```cpp
void SerialConfigWindow::on_openSerialButton_clicked()
```

Qt 会根据对象名称自动连接对应的槽函数。

### 方式二：构造函数手动 connect

例如：

```cpp
connect(ui->openSerialButton, &QPushButton::clicked, ...);
```

两种方式同时存在时，一个按钮点击事件可能触发两次业务逻辑。

因此进行了整理。

最终原则：

```text
普通界面按钮
    → 使用 Qt Designer 自动连接槽函数

模块状态信号
    → 使用 connect() 手动连接
```

即：

- 删除按钮相关的重复 `connect()`；
- 保留 `DeviceManager` 状态信号的手动连接；
- 由 `on_xxx_clicked()` 统一处理按钮业务逻辑。

这样可以避免同一按钮事件被重复处理。

---

## 七、串口设备检测情况

实际运行程序时发现当前电脑没有检测到可用串口设备。

串口下拉框为空。

经过排查，目前判断该现象与程序本身的串口列表刷新逻辑不一定有关，而是当前电脑没有可被系统识别的串口设备。

可能原因包括：

- 当前电脑没有物理串口；
- 没有连接 USB 转串口设备；
- 没有安装对应串口设备驱动；
- 当前没有创建虚拟串口。

因此目前无法直接进行真实硬件串口通信测试。

但串口数据处理链路已经可以通过模拟数据方式进行测试。

后续如果需要进行完整串口测试，可以使用：

- USB 转 TTL/RS232/RS485 模块；
- 虚拟串口工具；
- 两个虚拟串口组成的串口对；
- 实际下位机设备。

---

## 八、今日开发成果

今日主要完成了串口配置窗口从“界面存在”到“具备基本业务逻辑”的完善。

目前串口配置模块已经具备：

- [x] 串口列表刷新
- [x] 串口参数选择
- [x] 波特率配置
- [x] 数据位配置
- [x] 校验位配置
- [x] 停止位配置
- [x] 打开串口
- [x] 关闭串口
- [x] 串口状态显示
- [x] 串口连接状态控制
- [x] 无可用串口时的处理
- [x] 按钮重复信号连接问题排查

---

## 九、当前状态

当前串口通信模块的软件结构已经基本建立：

```text
串口配置窗口
      ↓
DeviceManager
      ↓
SerialPort
      ↓
QSerialPort
```

同时已经具备：

```text
串口数据
   ↓
SerialPort
   ↓
ProtocolParser
   ↓
DeviceManager
   ↓
Device
   ↓
MainWindow
   ↓
UI / SQLite / AlarmManager
```

因此后续工作的重点可以从“串口配置界面”进一步转向**串口通信实际联调**。

---

## 十、遗留问题与后续计划

### 遗留问题

目前尚未进行真实物理串口设备的完整测试。

主要原因是当前电脑没有可用串口设备。

因此以下内容暂未完成实际硬件验证：

- 真实串口打开；
- 真实串口发送；
- 真实串口接收；
- 实际设备数据解析；
- 真实设备在线/离线检测；
- 真实串口异常处理。

### 后续计划

下一阶段可以继续进行：

1. 使用模拟串口数据测试完整数据处理链路；
2. 检查串口模式下设备数据更新是否正常；
3. 测试串口超时后的设备离线判断；
4. 测试设备重新上线及报警恢复；
5. 条件允许时使用虚拟串口或 USB 转串口设备进行真实通信测试。

---
下面整理成两个部分：

1. `开发日志/day10.md`
2. Git commit 摘要

内容按照你当前项目《设备监控上位机系统》的开发节奏整理，重点突出**通信层架构完善**，避免写成流水账。

---

## day10.md

```md
# Day10 开发日志

## 日期
2026-09-29

## 今日开发内容

今日主要完成设备监控上位机系统通信层扩展，实现 Modbus RTU 与 Modbus TCP 两种工业通信协议的基础支持，并完善 DeviceManager 对不同通信方式的数据管理架构。

---

# 一、Modbus RTU 通信模块完善

## 1. Modbus RTU协议支持

完成 Modbus RTU 基础功能：

- CRC-16校验计算
- 读取保持寄存器（功能码 03）请求帧生成
- Modbus响应帧解析
- 异常响应处理
- CRC错误检测


支持：

- 从站地址校验
- 功能码校验
- 数据长度校验
- CRC校验


---

## 2. Modbus RTU接收缓冲处理

针对串口通信存在的数据分包、粘包问题，实现接收缓存机制。

新增：

- Modbus接收缓冲区
- 完整响应帧提取
- CRC校验后确认有效数据


通过模拟响应测试验证：

请求：

```
01 03 00 00 00 03 05 CB
```

模拟响应：

```
01 03 06 01 00 00 DC 00 01 20 9E
```


解析结果：

```
寄存器:
256
220
1
```


转换设备数据：

```
温度:25.6℃
电压:220V
在线:true
```


---

# 二、Modbus RTU设备轮询机制

完善 DeviceManager 中 Modbus RTU轮询逻辑。


实现：

- 根据设备列表动态获取设备ID
- 自动轮询设备
- 请求状态管理
- 响应等待机制
- 请求超时处理


支持：

```
Device1
 ↓
读取寄存器

Device2
 ↓
读取寄存器

Device3
 ↓
读取寄存器
```


不再固定设备编号，实现设备动态增删后的轮询。


---

# 三、Modbus TCP通信模块搭建

新增 Modbus TCP 通信类：

```
Communication/modbustcp.h
Communication/modbustcp.cpp
```


实现：

- QTcpSocket TCP连接
- TCP连接状态管理
- 连接错误处理
- 数据发送接口
- 接收信号通知


支持：

- TCP连接建立
- TCP断开
- TCP错误处理


---

# 四、Modbus TCP协议基础实现

完成 Modbus TCP请求构造。


支持MBAP Header：

```
Transaction ID
Protocol ID
Length
Unit ID
```

以及：

```
Function Code 03
Read Holding Registers
```


增加：

- Transaction ID记录
- Unit ID记录
- 请求状态管理


---

# 五、Modbus TCP接收架构设计

针对TCP字节流特性，设计TCP接收缓存方案。


由于TCP不存在天然数据帧，需要：

```
QTcpSocket
    |
readAll()
    |
m_buffer
    |
MBAP Length解析
    |
完整Modbus TCP帧
    |
DeviceManager
```


计划通过MBAP中的Length字段实现：

- TCP半包处理
- TCP粘包拆分
- 完整响应帧提取


---

# 六、通信架构调整

当前通信架构：

```
                 DeviceManager

          +----------------+
          |                |
       ModbusRTU       ModbusTCP
          |                |
      SerialPort       QTcpSocket
          |                |
          +-------+--------+
                  |
            DeviceData
                  |
              Device
```


通信层负责：

- 字节收发
- 协议解析


DeviceManager负责：

- 请求管理
- 设备映射
- 数据转换
- 状态更新


实现通信层与业务层解耦。


---

# 今日测试情况

已完成：

✅ Modbus RTU请求生成测试

✅ Modbus RTU响应解析测试

✅ CRC错误检测测试

✅ Modbus RTU设备轮询测试

✅ Modbus RTU数据更新流程测试


未测试：

- Modbus TCP真实连接
- TCP半包/粘包处理
- 多线程通信模型


---

# 明日开发计划

## 1. Modbus TCP通信测试

由于当前无真实设备，采用模拟TCP服务器方式测试：

计划：

- 创建TCP模拟从站
- 模拟MBAP响应
- 验证TCP数据解析
- 验证设备数据更新流程


测试：

```
Client
 |
ModbusTCP
 |
TCP Socket
 |
模拟设备
```


---

## 2. 完善TCP接收缓存

实现：

- MBAP长度解析
- 多帧连续接收
- 半包等待
- 粘包拆分


---

## 3. 引入通信多线程

优化通信架构：

当前：

```
UI线程
 |
DeviceManager
 |
通信
```


计划：

```
UI线程

    |
DeviceManager

    |
Communication Thread

    |
SerialPort / TCP Socket
```


避免：

- 串口阻塞UI
- TCP等待影响界面响应
- 大量设备轮询导致卡顿


---

## 4. 后续优化方向

- Modbus RTU/TCP统一通信接口
- 通信任务队列
- 请求超时重试机制
- 多设备并发管理
- 通信日志记录


---

# 总结

今日完成设备监控上位机通信层重要扩展。

系统已经从：

```
模拟数据驱动
```

逐步转变为：

```
工业通信协议驱动
```

完成 Modbus RTU基础通信能力，并搭建 Modbus TCP通信框架，为后续真实设备接入和多线程通信优化提供基础。
```

---

## Git Commit摘要

推荐：

```bash
git add .
git commit -m "feat: add Modbus RTU polling and Modbus TCP communication framework"
```

如果想拆分更符合工程习惯：

### Commit 1

```bash
git commit -m "feat: implement Modbus RTU register communication and polling"
```

内容：

- CRC16校验
- RTU请求生成
- 响应解析
- 轮询设备
- 模拟测试


### Commit 2

```bash
git commit -m "feat: add Modbus TCP communication module"
```

内容：

- QTcpSocket封装
- TCP连接管理
- MBAP请求构造
- TCP响应接口


### Commit 3（如果明天完成）

```bash
git commit -m "feat: improve Modbus TCP frame parsing and communication threading"
```

内容：

- TCP缓存
- MBAP拆包
- 通信线程


---

今天这个节点实际上是一个比较明显的阶段：

**Day1-9：上位机框架 + 数据管理 + UI + 数据库**

↓

**Day10：工业通信协议接入**

下一阶段重点就是：

**真实通信模拟 → 多线程通信 → UI美化 → 文档整理 → 简历项目包装**

已经比较接近一个完整工业上位机项目的形态。

# 设备监控上位机开发日志

**日期：2026年9月30日**

## 一、今日开发主题

今日主要围绕 **Modbus TCP 通信功能完善、多线程通信架构、TCP 请求状态管理以及后续 UI 功能规划** 展开开发。

重点解决 Modbus TCP 在独立线程中的通信问题，并进一步完善 TCP 请求与 RTU 请求之间的状态隔离，为后续设备管理和系统集成测试做准备。

---

## 二、Modbus TCP 多线程通信改造

### 1. 建立 TCP 独立工作线程

将 `ModbusTCP` 通信对象移动至独立的 `QThread` 中运行，使 `QTcpSocket` 的网络通信操作与主线程中的 UI 和设备管理逻辑分离。

整体结构调整为：

```text
主线程
│
├── MainWindow
├── DeviceManager
│
└── 通过信号/槽与 TCP 工作线程通信
          │
          ▼
      QThread
          │
          └── ModbusTCP
                 │
                 └── QTcpSocket
```

这样可以避免网络通信过程对 Qt 主线程造成阻塞，同时为后续长时间轮询和多设备通信提供更合理的线程结构。

### 2. 使用信号/槽进行跨线程请求

将原本可能直接调用 TCP 对象成员函数的方式调整为通过 Qt 信号向工作线程投递请求。

通过类似：

```cpp
sendTCPReadRequest(
    transactionId,
    unitId,
    startAddress,
    quantity
);
```

将请求从 `DeviceManager` 所在线程投递到 `ModbusTCP` 工作线程。

同时要求实际执行网络操作的函数作为 Qt `slot` 或 `Q_INVOKABLE` 使用，使跨线程调用能够通过 Qt 的事件队列正确执行。

### 3. TCP 数据通过信号返回主线程

ModbusTCP 工作线程完成数据接收和完整 MBAP 报文解析后，通过信号将数据发送回 `DeviceManager`。

通信结构调整为：

```text
DeviceManager
      │
      │ sendTCPReadRequest
      ▼
ModbusTCP 工作线程
      │
      │ QTcpSocket
      ▼
   TCP设备
      │
      │ Modbus TCP响应
      ▼
ModbusTCP
      │
      │ dataReceived
      ▼
DeviceManager
```

主线程负责设备状态和数据处理，TCP 工作线程负责 Socket 通信，从而实现通信职责与业务逻辑的分离。

### 4. 完善线程退出流程

对 TCP 工作线程的退出流程进行了处理。

程序关闭时：

1. 向 TCP 工作对象发送关闭请求。
2. 关闭 `QTcpSocket`。
3. 调用线程 `quit()`。
4. 使用 `wait()` 等待线程真正结束。
5. 通过 `finished -> deleteLater` 清理工作对象。

此前出现的：

```text
QThread: Destroyed while thread is still running
```

问题已经通过调整线程退出流程得到解决，用户确认程序已经能够正常退出。

---

## 三、Modbus TCP 通信功能完善

### 1. 增加 TCP 发送结果反馈

为 TCP 请求增加发送结果反馈机制：

```cpp
sendResult(transactionId, success)
```

用于区分：

- 请求是否成功写入 `QTcpSocket` 的发送缓冲区；
- 请求发送失败。

同时明确：

> `write()` 成功并不代表远端设备已经收到并处理请求，只能说明数据已经成功交给本地 Socket 发送缓冲区。

因此 TCP 请求仍然需要等待实际响应，并由超时机制负责处理无响应情况。

### 2. 增加 TCP 连接状态反馈

完善 TCP 连接状态信号：

```text
tcpConnected()
tcpDisconnected()
tcpError(QString)
```

由 `DeviceManager` 接收后维护 TCP 当前连接状态。

引入：

```cpp
m_tcpConnected
```

用于判断当前 TCP 是否处于可通信状态。

TCP 未连接时，不继续执行 TCP 轮询请求。

### 3. 增加独立 TCP 超时机制

将 TCP 请求超时处理从 RTU 请求中独立出来。

TCP 使用：

```text
m_tcpRequestPending
m_tcpTimeoutTimer
m_tcpExpectedTransactionId
m_tcpExpectedUnitId
m_tcpExpectedQuantity
```

RTU 继续使用原有：

```text
m_modbusRequestPending
m_modbusTimeoutTimer
m_expectedDeviceId
m_expectedQuantity
```

这样可以避免 RTU 和 TCP 共用请求状态导致通信相互干扰。

---

## 四、Modbus TCP 请求状态隔离

今日进一步检查了 RTU 和 TCP 的请求生命周期。

### RTU

RTU 请求继续使用：

```cpp
m_modbusRequestPending
m_modbusTimeoutTimer
```

`requestModbusRead()` 当前逻辑保持不变。

其主要流程为：

```text
检查RTU协议
      ↓
检查串口
      ↓
检查是否存在未完成请求
      ↓
构造RTU请求帧
      ↓
记录请求设备
      ↓
发送请求
      ↓
启动RTU超时计时器
```

### TCP

TCP 使用独立状态：

```text
m_tcpRequestPending
m_tcpTimeoutTimer
m_tcpExpectedTransactionId
m_tcpExpectedUnitId
m_tcpExpectedQuantity
```

避免 TCP 请求清理 RTU 状态，或者 RTU 请求影响 TCP 超时处理。

---

## 五、Modbus TCP 响应处理完善

检查并调整了 `handleModbusTCPResponse()` 的处理逻辑。

TCP 响应首先检查：

1. 是否存在正在等待的 TCP 请求；
2. 数据长度是否满足基本要求；
3. Transaction ID 是否与当前请求匹配；
4. Modbus TCP 响应报文是否能够正确解析。

只有收到当前请求对应的 Transaction ID 后，才结束当前 TCP 请求等待状态。

处理流程：

```text
收到TCP响应
    ↓
检查TCP请求状态
    ↓
检查Transaction ID
    ↓
解析Modbus TCP报文
    ↓
停止TCP超时计时器
    ↓
清除TCP请求状态
    ↓
处理寄存器数据
```

对于 Transaction ID 不匹配的报文，不立即清除当前请求状态，继续等待正确响应。

---

## 六、TCP 异常和断线处理

### 1. TCP 发送失败

当 TCP 请求发送失败时：

```text
停止TCP超时计时器
      ↓
清除m_tcpRequestPending
      ↓
允许后续重新发起请求
```

不会修改 RTU 请求状态。

### 2. TCP 连接断开

TCP 断开时：

```text
m_tcpConnected = false
m_tcpTimeoutTimer->stop()
m_tcpRequestPending = false
```

避免断线后仍然保持一个无效的 TCP 请求。

### 3. TCP 迟到响应

如果设备已经删除或者 TCP 请求已经被取消，后续收到旧请求对应的响应时，不再继续处理已经失效的请求。

---

## 七、轮询机制调整

检查了 `pollNextDevice()`。

当前已经按照通信协议分别判断请求状态：

```cpp
if (m_protocolType == ProtocolType::ModbusRTU &&
    m_modbusRequestPending) {
    return;
}

if (m_protocolType == ProtocolType::ModbusTCP &&
    m_tcpRequestPending) {
    return;
}
```

因此 RTU 和 TCP 不再通过同一个 pending 状态进行判断。

同时规划增加：

```cpp
if (m_protocolType == ProtocolType::ModbusTCP &&
    !m_tcpConnected)
{
    return;
}
```

避免 TCP 未连接时继续执行轮询请求。

当前轮询架构仍然通过：

```cpp
m_protocolType
```

选择当前使用的通信协议，因此目前是 RTU/TCP 二选一的轮询模式，并不是不同设备同时使用不同通信协议进行并行轮询。

---

## 八、设备删除与通信状态清理

检查了 `removeDevice()`。

原有代码已经能够在删除正在等待 RTU 响应的设备时：

- 停止 RTU 超时计时器；
- 清除 `m_modbusRequestPending`；
- 清空 Modbus RTU 接收缓存。

进一步增加 TCP 设备删除时的状态清理方案：

```text
判断当前等待设备是否为被删除设备
          ↓
停止m_tcpTimeoutTimer
          ↓
m_tcpRequestPending = false
```

从而避免设备已经从设备列表删除，但程序仍然保持等待其 TCP 响应的状态。

---

# 九、当前 Modbus TCP 开发状态

经过今天的开发，Modbus TCP 已经基本形成完整的通信架构：

```text
┌──────────────────────────────┐
│          MainWindow          │
│          主线程 UI            │
└──────────────┬───────────────┘
               │
               ▼
┌──────────────────────────────┐
│        DeviceManager         │
│     设备管理 / 轮询 / 数据处理 │
└──────────────┬───────────────┘
               │ Signal / Slot
               ▼
┌──────────────────────────────┐
│        ModbusTCP Thread      │
│        独立通信线程            │
└──────────────┬───────────────┘
               │
               ▼
┌──────────────────────────────┐
│          QTcpSocket           │
│          TCP通信               │
└──────────────────────────────┘
```

目前核心通信功能已经基本具备，但**暂不进行最终集成测试**，后续仍需要在 UI、设备管理和报警功能完成后统一进行系统级验证。

---

# 十、下一阶段开发计划

当前不立即进入全面集成测试，而是继续完善上位机功能和 UI。

### 1. UI 完善

- 主界面整体 UI 美化
- 调整设备列表布局
- 优化设备状态显示
- 优化设备详情界面
- 完善报警区域

### 2. 设备管理功能

增加：

- 删除设备
- 添加 Modbus RTU 设备
- 添加 Modbus TCP 设备
- 完善设备配置入口
- 显示设备通信来源

设备来源需要能够明确区分：

```text
自定义串口
Modbus RTU
Modbus TCP
```

### 3. 报警功能

完善：

- 报警状态显示
- 报警界面闪烁
- 异常设备提示
- 温度、电压等数据异常提示
- 报警解除后的状态恢复

### 4. 最终集成测试

以上功能完成后，再统一进行：

- 自定义串口通信测试
- Modbus RTU 通信测试
- Modbus TCP 通信测试
- 设备添加/删除测试
- 多设备轮询测试
- TCP断线测试
- RTU超时测试
- TCP超时测试
- 异常响应测试
- 报警功能测试
- 多线程退出测试
- 长时间运行稳定性测试

---

# 十一、今日开发总结

今日开发重点由单纯的 Modbus TCP 功能实现，进一步进入了**通信架构完善阶段**。

主要完成和推进了：

1. Modbus TCP 独立线程通信架构；
2. TCP 请求通过信号/槽进行跨线程调度；
3. TCP 数据接收后的跨线程回传；
4. TCP 工作线程安全退出；
5. TCP 发送结果反馈；
6. TCP 连接状态反馈；
7. TCP 独立超时机制；
8. TCP 与 RTU 请求状态隔离；
9. TCP Transaction ID 校验；
10. TCP 断线及发送失败状态清理；
11. TCP 设备删除时的请求状态清理；
12. `pollNextDevice()` 的 RTU/TCP 请求状态区分；
13. 明确后续 UI、设备管理和报警功能开发顺序；
14. 明确在主要功能完成后统一开展系统集成测试。

下一阶段重点从**通信底层开发**逐步转向**设备管理、UI 和用户交互功能完善**。

---

## Git Commit

```bash
git add .
git commit -m "完善Modbus TCP多线程通信与请求状态管理"
```

如果今天你的实际代码已经把上述修改全部提交并编译通过，这个 commit 摘要比较合适；如果部分内容只是今天讨论的修改方案而尚未真正落地，建议不要把它们写成“已完成”的提交内容。

# 《设备监控上位机系统》开发日志

## 2026年9月30日

### 一、Modbus TCP模拟服务器开发

今日开始完善 Modbus TCP 通信测试环境。由于目前缺少实际工业设备用于持续联调，因此首先实现 `MockModbusTCPServer`，在本机模拟 Modbus TCP 从设备，为上位机提供可控的测试对象。

模拟服务器采用 `QTcpServer` 接收上位机连接，并使用 `QTcpSocket` 处理客户端请求。针对 TCP 数据可能出现分包和粘包的问题，在服务器端增加接收缓冲区，根据 MBAP Header 中的 Length 字段判断一帧 Modbus TCP 报文是否完整，完整后再进行解析。

模拟服务器目前支持功能码 `03`，用于读取保持寄存器，并模拟温度、电压和在线状态三个寄存器。

### 二、遇到的问题及解决

**问题1：TCP 数据不能简单按照一次 `readyRead()` 处理。**

TCP 是字节流协议，一次 `readyRead()` 收到的数据不一定刚好对应一个完整 Modbus TCP 请求。

**解决方法：**

增加每个客户端对应的接收缓冲区：

```text
TCP数据
  ↓
追加到缓冲区
  ↓
读取MBAP Length
  ↓
判断完整帧长度
  ↓
提取完整Modbus TCP帧
  ↓
解析请求
```

这样可以处理一次接收半帧以及一次接收多帧的情况。

**问题2：模拟服务器的 Modbus TCP 报文校验不够完整。**

初始实现主要关注功能码和数据内容，后续发现还需要对 Protocol ID、Length、功能码、寄存器地址以及读取数量进行更加严格的判断。

**解决方法：**

增加 MBAP Header 校验和异常响应机制，对非法功能码、非法数据值和非法数据地址分别生成对应的 Modbus 异常响应。

本阶段完成 Git 提交：

- `481b401`：实现ModbusTCP模拟服务器通讯

---

# 2026年10月1日

## 一、完善 Modbus TCP 多线程通信与请求状态管理

随着 Modbus TCP 模拟服务器建立，开始进一步完善上位机通信模块。

此前 Modbus TCP 通信主要能够完成基本的连接、发送和接收，但随着通信流程复杂化，需要区分连接状态、发送状态和响应状态，否则容易出现请求发送后无法准确判断对应响应的问题。

因此对 `ModbusTCP` 和 `DeviceManager` 进行了调整，引入事务 ID 对请求和响应进行匹配，并增加通信状态信号。

### 二、遇到的问题及解决

**问题1：发送请求后，无法准确管理请求是否成功。**

单纯依靠 `sendReadHoldingRegistersRequest()` 的返回值，只能判断数据是否成功写入 Socket，不能说明下位机是否已经返回正确响应。

**解决方法：**

增加请求结果信号，并使用 Modbus TCP 的 Transaction ID 区分请求：

```text
发送请求
    ↓
记录 Transaction ID
    ↓
等待响应
    ↓
解析响应 Transaction ID
    ↓
匹配对应请求
```

同时增加连接成功、连接断开和 TCP 错误等状态信号。

**问题2：响应报文的完整性检查不足。**

发现仅检查报文长度和 Transaction ID 还不够，MBAP Length 字段也必须与实际报文长度匹配。

**解决方法：**

增加：

- 响应最小长度检查；
- Transaction ID 检查；
- Protocol ID 检查；
- MBAP Length 检查；
- 响应数据合法性检查。

从而避免错误报文被当作正常数据处理。

本阶段完成 Git 提交：

- `e9699dd`：完善Modbus TCP多线程通信与请求状态管理

---

# 2026年10月2日

## 一、系统集成

今日将前期已经完成的通信模块、设备对象、设备配置和界面进行进一步整合。

对 `Device`、`DeviceConfig`、`DeviceManager` 和 `MainWindow` 之间的数据流进行了统一，使设备对象不仅保存设备数据，还能够保存设备名称、通信协议和完整设备配置。

同时完善 AddDeviceDialog，使不同协议能够对应不同的配置参数。

### 二、遇到的问题及解决

**问题1：设备对象之前只保存运行数据，无法完整描述设备的通信配置。**

这样会导致 MainWindow 和 DeviceManager 无法准确判断某个设备采用的是哪种协议和数据源。

**解决方法：**

引入统一的 `DeviceConfig`，将：

- 数据源；
- 协议类型；
- 串口参数；
- Modbus RTU参数；
- Modbus TCP参数

统一保存到设备配置中。

DeviceManager 根据每个 Device 自身的配置决定通信方式，而不是简单依赖全局通信状态。

**问题2：原有模拟数据更新逻辑与真实通信设备逻辑耦合。**

模拟设备和真实通信设备都需要更新 Device，但两者的数据来源不同。

**解决方法：**

调整 `updateAllDevices()`，根据每个 Device 的 `DataSource` 单独判断：

```text
Simulation → 使用模拟数据
Serial/TCP → 等待通信数据
```

避免通信设备被错误地使用本地模拟数据覆盖。

本阶段完成 Git 提交：

- `42ae83c`：集成测试

---

# 2026年10月3日

## 一、Modbus TCP数据流测试及功能补充

今日重点对 Modbus TCP 的完整数据流进行测试。

测试过程中进一步完善模拟服务器，使其能够按照 Modbus TCP 标准处理异常请求，并增加异常功能码、非法数据值以及非法数据地址等情况。

### 二、遇到的问题及解决

**问题1：模拟服务器对于非法请求的处理不够规范。**

原来的模拟服务器只针对正常的功能码03请求进行处理，对于非法请求没有统一的异常响应。

**解决方法：**

增加统一的异常响应生成逻辑：

```text
正常请求 → 正常数据响应

非法功能码 → 01 Illegal Function
非法数据值 → 03 Illegal Data Value
非法数据地址 → 02 Illegal Data Address
```

这样可以更接近真实 Modbus TCP 从设备的行为。

**问题2：寄存器地址边界判断存在不足。**

原先通过：

```text
startAddress + quantity > registers.size()
```

判断范围，在边界情况下不够严谨。

**解决方法：**

改为分别检查起始地址和读取数量，确保：

```text
startAddress < registers.size()
quantity <= registers.size() - startAddress
```

防止寄存器访问越界。

同时继续完善设备配置和数据流测试。

本阶段完成 Git 提交：

- `f7e2c04`：Modbus TCP数据流测试及功能性补充

---

# 2026年10月4日

## 一、Modbus TCP多设备通信开发与测试

今日开始进行 Modbus TCP 多设备通信测试。

在前期单设备通信已经能够运行的基础上，进一步验证多个逻辑设备同时加入系统后的通信情况。

设备之间通过不同的 Unit ID 进行区分，而不是为每一个逻辑设备单独建立 TCP Socket。

### 二、遇到的问题及解决

**问题：多个设备加入后，通信请求和设备配置之间容易产生混淆。**

如果简单按照设备数量创建 TCP 连接，会造成连接管理复杂，同时不符合 Modbus TCP 常见的设备组织方式。

**解决方法：**

采用：

```text
一个TCP连接
    ↓
多个逻辑设备
    ↓
不同Unit ID
```

DeviceManager 根据每个 Device 的 TCP 配置生成对应请求，在轮询过程中使用不同 Unit ID 区分设备。

同时对设备轮询流程进行调整，使通信设备和纯模拟设备按照各自的数据源进行处理。

经过测试，多设备通信能够按照不同设备配置进行请求和数据更新。

本阶段完成 Git 提交：

- `b48069c`：tcp多设备通信开发&测试

---

# 2026年10月5日

## 一、增加 Modbus TCP 超时与重连机制

在多设备通信测试后，进一步考虑实际工业通信中的异常情况。

正常情况下，TCP连接建立后可以持续通信，但实际设备可能出现：

- 网络断开；
- 设备停止响应；
- 请求发送后长时间没有响应；
- Socket异常断开。

因此今日重点增加请求超时和 TCP 自动重连机制。

### 二、遇到的问题及解决

**问题1：发送请求后，如果设备没有响应，请求会一直处于等待状态。**

此前请求主要依赖正常响应完成状态更新，如果设备不返回数据，上位机无法及时判断此次请求失败。

**解决方法：**

为每一个 Transaction ID 建立对应的超时计时器。

```text
发送请求
    ↓
记录Transaction ID
    ↓
启动1秒超时计时器
    ↓
收到响应
    ↓
停止计时器并删除请求状态

如果超时：
    ↓
触发requestTimeout
    ↓
删除pending request
```

从而使每个请求都具有明确的生命周期。

**问题2：TCP连接断开后不会自动恢复。**

实际运行过程中，如果 TCP Socket 断开，需要重新连接设备，否则后续轮询请求将持续失败。

**解决方法：**

增加重新连接定时器，TCP断开后启动定时重连机制，每隔一定时间检查连接状态并尝试重新连接。

同时保存最近一次连接使用的 IP 和端口，使断开后能够重新使用原连接参数。

**问题3：模拟服务器停止时，已经建立的客户端连接没有统一清理。**

**解决方法：**

停止模拟服务器时，主动断开所有已经建立的客户端连接，关闭 Socket，清理客户端缓冲区，然后停止服务器监听。

本阶段完成 Git 提交：

- `8926c2f`：modbusTCP超时重连功能添加

---

# 2026年10月6日

## 一、自定义串口通信开发

在 Modbus TCP 通信基本完成后，开始进行自定义串口通信功能开发。

首先明确自定义串口通信采用**上位机主动轮询、下位机响应**的通信方式，而不是由模拟设备主动向上位机推送数据。

自定义请求帧：

```text
AA 设备ID 01 55
```

自定义响应帧：

```text
AA 设备ID 温度 电压高字节 电压低字节 在线状态 55
```

其中：

- `AA`：帧头；
- `设备ID`：设备编号；
- `01`：读取数据命令；
- 温度：当前温度；
- 电压：16位电压数据；
- 在线状态：设备在线状态；
- `55`：帧尾。

## 二、开发模拟串口下位机

由于目前没有真实串口设备用于联调，因此增加 `MockSerialDevice` 模拟下位机。

测试时使用虚拟串口对连接两个程序：

```text
MockSerialDevice
     │
    COM1
     │
虚拟串口对
     │
    COM2
     │
DeviceManager
```

其中：

- COM1：模拟下位机使用；
- COM2：上位机 DeviceManager 使用。

同时增加独立的模拟串口配置窗口，用于配置 MockSerialDevice 的串口参数。

### 三、遇到的问题及解决

**问题1：模拟下位机原有的数据接收逻辑与实际请求帧长度不一致。**

上位机发送的是4字节请求：

```text
AA 01 01 55
```

而模拟设备之前的接收处理逻辑按照7字节响应帧进行解析，因此无法正确识别上位机请求。

**解决方法：**

重新设计 MockSerialDevice 的接收缓冲处理：

```text
接收数据
    ↓
进入缓冲区
    ↓
寻找 AA
    ↓
等待4字节完整请求
    ↓
验证55
    ↓
解析设备ID
    ↓
生成7字节响应
```

### 四、解决上位机与模拟下位机串口配置职责混淆问题

开发过程中进一步明确了真实工业通信中的串口职责：

> 上位机只负责打开和管理自己这一侧的通信端口，并不存在“上位机去打开下位机串口”的情况。

因此没有将 MockSerialDevice 的串口配置加入正常 AddDevice 流程，而是设计独立的模拟下位机配置窗口。

最终形成：

```text
AddDeviceDialog
    ↓
配置上位机侧串口
    ↓
DeviceManager
```

以及：

```text
MockSerialConfigWindow
    ↓
配置模拟下位机串口
    ↓
MockSerialDevice
```

两者职责独立。

### 五、解决 Qt Designer 自动连接问题

增加 MockSerialConfigWindow 后，运行过程中出现：

```text
QMetaObject::connectSlotsByName:
No matching signal for on_mockSerialConfigBtn_clicked()
```

检查后发现槽函数命名与 Qt Designer 中按钮的 `objectName` 不一致。

将按钮的 `objectName` 与槽函数：

```cpp
on_mockSerialConfigBtn_clicked()
```

对应起来后，Qt 的自动连接恢复正常。

### 六、自定义串口通信联调结果

目前已经完成完整请求—响应链路测试。

实际测试过程：

```text
上位机发送：
AA 01 01 55

        ↓

MockSerialDevice收到：
AA 01 01 55

        ↓

MockSerialDevice响应：
AA 01 1A 00 E2 01 55

        ↓

上位机解析：
设备ID = 1
温度 = 26℃
电压 = 226V
在线 = true

        ↓

DeviceManager转发

        ↓

MainWindow更新
```

连续测试过程中，温度和电压能够随着模拟数据变化而更新，说明自定义串口的：

- 请求发送；
- 请求解析；
- 响应生成；
- 响应接收；
- 协议解析；
- DeviceManager转发；
- MainWindow显示

已经完整打通。

目前自定义串口通信基本完成。

---

# 2026年10月7日开发计划

## Modbus RTU通信测试

明日进入 Modbus RTU 通信测试阶段。

重点验证：

1. 串口参数配置；
2. Slave ID；
3. Modbus RTU请求帧；
4. 功能码；
5. 起始寄存器地址；
6. 读取数量；
7. CRC校验；
8. 响应帧接收；
9. Modbus RTU数据解析；
10. DeviceManager数据更新；
11. MainWindow数据显示。

测试目标：

```text
DeviceManager
      ↓
Modbus RTU请求
      ↓
模拟/真实RTU设备
      ↓
RTU响应
      ↓
CRC校验
      ↓
数据解析
      ↓
DeviceManager
      ↓
Device
      ↓
MainWindow
```

首先完成**单设备完整通信链路测试**，确认请求帧、响应帧和 CRC 均正确后，再进行多设备轮询测试。

# 设备监控上位机系统开发日志

**日期：2026年10月7日**

## 一、今日开发内容

### 1. 完成 Modbus RTU 模拟设备通信测试

完善 `MockModbusRTUDevice`，实现基于串口的 Modbus RTU 从站模拟功能。

主要完成：

- 实现 Modbus RTU 请求帧接收；
- 实现请求帧 CRC-16/Modbus 校验；
- 解析从站地址、功能码、起始寄存器地址及读取数量；
- 支持功能码 `0x03`（读保持寄存器）；
- 根据请求构造 Modbus RTU 响应帧；
- 实现响应数据 CRC 计算；
- 通过串口返回模拟设备数据。

经过实际测试，Modbus RTU 的数据通信链路已经打通，可以实现上位机发送请求、模拟从站解析请求并返回数据、上位机接收并解析数据的完整流程。

### 2. 完善 Modbus RTU 模拟数据

此前 `MockModbusRTUDevice` 使用固定的周期性数值变化方式生成模拟数据，数据变化规律较为明显，不利于模拟真实设备运行状态。

今日将模拟数据生成方式调整为调用项目统一的 `SimulationData` 工具类。

当前模拟数据生成方式：

- 温度：根据当前值进行随机小幅变化，限制在 `15.0～50.0℃`；
- 电压：根据当前值随机变化 `±1V`，限制在 `210～230V`；
- 在线状态：当前固定为在线状态。

其中温度数据采用 `0.1℃` 为单位，例如 `253` 表示 `25.3℃`。

### 3. 统一不同通信协议的模拟数据来源

明确模拟设备的数据生成与通信协议解耦。

当前结构为：

```text
                    SimulationData
                         │
             ┌───────────┴───────────┐
             ↓                       ↓
      MockSerialDevice       MockModbusRTUDevice
             │                       │
       自定义串口协议             Modbus RTU
             │                       │
             └──────────┬────────────┘
                        ↓
                   DeviceManager
                        ↓
                    MainWindow
```

不同协议的模拟设备只负责**协议帧的封装与解析**，具体的温度、电压、在线状态等模拟数据统一由 `SimulationData` 提供。

这样可以避免不同模拟设备各自维护一套随机数据生成逻辑，保证后续增加其他通信协议时能够复用相同的数据模拟机制。

## 二、今日开发成果

截至今日，项目已经完成：

- 自定义串口通信链路测试；
- Modbus RTU 通信链路测试；
- Modbus TCP 通信架构；
- Modbus RTU CRC 校验及数据解析；
- Modbus RTU 模拟从站；
- 自定义串口与 Modbus RTU 模拟数据统一；
- 基于当前值的小幅随机模拟数据。

目前项目已经具备三种通信方式的基本设备接入框架：

| 通信方式 | 状态 |
|---|---|
| 自定义串口 | 已打通 |
| Modbus RTU | 已打通 |
| Modbus TCP | 架构及通信功能已完成 |
| 模拟数据统一 | 已完成 |

## 三、问题与改进

今日发现原有 Modbus RTU 模拟数据采用固定规律变化，模拟效果较为机械。

通过引入 `SimulationData` 统一数据生成逻辑，将模拟数据生成与具体通信协议进行解耦，使模拟设备更加接近真实设备的随机波动特征，同时提高了代码复用性和可维护性。

## 四、下一步计划

后续重点转向：

1. 完善上位机 UI；
2. 完善设备状态、报警等功能；
3. 完善设备历史数据及趋势显示；
4. 完成三种通信方式的系统集成测试；
5. 根据最终功能完善需求分析及设计文档。
