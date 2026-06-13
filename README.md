# 上海地铁换乘指南系统

这是一个基于 **C++ / Qt Widgets** 开发的上海地铁换乘指南系统，也是数据结构与算法设计课程作业项目。

系统使用图结构存储地铁线路和站点信息，支持地铁网络图形化显示、动态添加线路和站点，并能够根据用户输入的起点站和终点站，自动生成换乘指南，同时在图形界面中高亮显示乘车路径。

## 项目简介

本项目模拟上海地铁换乘查询场景，通过地铁站点和线路之间的连接关系构建地铁网络图。

用户可以在图形界面中选择起点站和终点站，系统会自动计算推荐乘车路径，并输出包括起点站、换乘站、终点站在内的换乘指南。

项目主要用于数据结构与算法课程设计，重点体现图的存储、图的遍历、最短路径搜索以及 Qt 图形界面编程。

## 功能特点

- 图形化显示上海地铁网络结构
- 支持动态添加地铁线路
- 支持动态添加地铁站点
- 支持站点之间建立连接关系
- 支持起点站和终点站路径查询
- 自动生成地铁换乘指南
- 支持乘车路径高亮显示
- 路径搜索优先考虑最少换乘次数
- 在换乘次数相同时，优先选择经过站点数更少的路线

## 已包含线路

项目初始数据中包含多条上海轨道交通线路，包括：

- 1号线
- 2号线
- 3号线
- 4号线
- 5号线
- 6号线
- 7号线
- 8号线
- 9号线
- 10号线
- 11号线
- 12号线
- 13号线
- 14号线
- 15号线
- 16号线
- 17号线
- 18号线
- 市域机场线
- 磁浮线
- 金山铁路

> 说明：项目中的线路、站点和坐标主要用于课程设计展示，不保证与上海地铁实时运营数据完全一致。

## 技术栈

- C++
- Qt Widgets
- CMake
- C++17
- 图结构
- 邻接表
- Dijkstra 算法

## 数据结构设计

本项目将地铁交通网络抽象为一个无向图。

### 站点结构

每个站点包含以下信息：

- 站点名称
- 站点在图形界面中的坐标
- 该站点所属的线路集合

示例：

```cpp
struct Station {
    QString name;
    QPointF pos;
    QSet<QString> lines;
};
```

### 边结构

每条边表示两个相邻站点之间的连接关系，并记录该连接所属的地铁线路。

```cpp
struct Edge {
    QString to;
    QString line;
};
```

### 图结构

地铁网络使用邻接表进行存储：

```cpp
QMap<QString, QVector<Edge>> adj;
```

其中：

- `QString` 表示站点名称
- `QVector<Edge>` 表示当前站点连接到的所有相邻站点

邻接表适合存储地铁网络这类稀疏图，方便进行路径搜索和动态扩展。

## 路径查询算法

项目中使用改进的 Dijkstra 算法进行路径查询。

普通最短路径算法通常只考虑经过的边数，但地铁出行中更重要的是换乘次数。因此本项目将搜索状态设计为：

```text
当前站点 + 当前乘坐线路
```

路径搜索的优先级为：

1. 换乘次数最少
2. 在换乘次数相同的情况下，经过站点数最少

这样可以让查询结果更接近实际乘坐地铁时的需求。

## 项目界面说明

程序主界面主要分为两个区域：

1. 左侧：地铁网络图显示区域
2. 右侧：功能操作区域

右侧功能区包含：

- 起点站选择框
- 终点站选择框
- 路径查询按钮
- 添加线路功能
- 添加站点功能
- 换乘指南显示框

查询路径后，系统会在左侧图形界面中高亮显示推荐乘车路径，并在右侧文本框中输出详细乘车方案。

## 运行环境

推荐环境：

- Qt 6.x
- Qt Creator
- CMake
- 支持 C++17 的编译器

Windows 下可以使用 Qt Creator 自带的 MinGW 或 MSVC 编译环境。

## 编译运行方法

### 方法一：使用 Qt Creator

1. 打开 Qt Creator
2. 选择 `Open Project`
3. 打开项目中的 `CMakeLists.txt`
4. 配置 Qt Kit
5. 点击构建并运行

### 方法二：使用命令行

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

构建完成后运行生成的可执行文件。

## CMake 配置示例

```cmake
cmake_minimum_required(VERSION 3.16)

project(ShanghaiMetroGuide VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(ShanghaiMetroGuide
    main.cpp
)

target_link_libraries(ShanghaiMetroGuide PRIVATE Qt6::Widgets)
```

如果项目使用 Qt Creator 自动生成的 CMake 文件，只需要保证 `main.cpp` 参与编译即可。

例如：

```cmake
set(PROJECT_SOURCES
    main.cpp
)
```

如果项目中仍然保留了 Qt 默认生成的 `mainwindow.cpp`、`mainwindow.h`、`mainwindow.ui`，但程序窗口类已经写在 `main.cpp` 中，则不要再把这些文件加入编译，否则可能出现 `multiple definition of MainWindow` 的重复定义错误。

## 使用方法

1. 启动程序
2. 在起点站下拉框中选择出发站
3. 在终点站下拉框中选择目的站
4. 点击“生成换乘指南并高亮路径”
5. 系统会显示推荐乘车方案
6. 左侧地铁图会高亮显示乘车路径

也可以通过界面中的“动态添加地铁线路”和“动态添加地铁站点”功能扩展地铁网络。

## 示例

例如查询：

```text
起点站：莘庄
终点站：陆家嘴
```

系统会输出类似结果：

```text
起点站：莘庄
终点站：陆家嘴
经过站数：若干
换乘次数：若干

乘车方案：
1. 在【莘庄】乘坐【1号线】。
2. 在【人民广场】换乘【2号线】。
3. 到达终点【陆家嘴】。
```

实际输出结果会根据当前程序中的线路和站点数据计算得到。

## 项目结构

一个简化后的项目结构如下：

```text
ShanghaiMetroGuide/
├── CMakeLists.txt
├── main.cpp
└── README.md
```

如果使用 Qt Creator 默认模板创建项目，可能会生成：

```text
mainwindow.cpp
mainwindow.h
mainwindow.ui
```

本项目当前版本将主要代码集中在 `main.cpp` 中，因此这些默认窗口文件不是必须的。

## 项目特点

- 将地铁网络抽象为图结构
- 使用邻接表存储站点连接关系
- 结合线路状态进行路径搜索
- 支持最少换乘优先的路径查询
- 使用 Qt Graphics View 实现图形化显示
- 支持动态添加线路和站点
- 适合作为数据结构与算法课程设计项目

## 后续改进方向

- 使用 JSON 或 CSV 文件保存线路与站点数据
- 增加真实地铁数据导入功能
- 增加票价计算功能
- 增加预计乘车时间计算
- 支持多条候选路线展示
- 支持最少站点、最少换乘、最短时间等不同查询策略
- 增加站点搜索框
- 增加地图缩放和拖拽优化
- 增加线路编辑和站点删除功能
- 将界面逻辑和图算法逻辑拆分成多个源文件

## 注意事项

本项目主要用于课程学习和算法演示。

如果项目中使用了第三方地铁线路图作为背景图片，请注意图片版权问题。若要将项目公开发布到 GitHub，建议：

- 使用自己绘制的示意图
- 或移除第三方地图背景
- 或确认图片来源允许公开使用
- 或在 README 中注明图片来源和使用目的

## License

This project is for course design and learning purposes only.

If third-party map images or third-party metro data are used, please make sure you have the right to redistribute them.
