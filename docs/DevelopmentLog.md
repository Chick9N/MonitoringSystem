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
