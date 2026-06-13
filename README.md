# 上海地铁换乘指南系统

基于 **C++ / Qt Widgets** 开发的上海地铁换乘指南系统，用于数据结构与算法课程设计。项目使用图结构存储地铁线路和站点信息，支持路径查询、换乘指南生成和图形化路径高亮。

如果喜欢，请留下一个Star⭐吧，谢谢喵~

## 功能简介

- 图形化显示上海地铁网络
- 动态添加地铁线路和站点
- 根据起点站、终点站查询乘车路径
- 自动生成换乘指南
- 高亮显示推荐乘车路线
- 查询策略：优先最少换乘，其次最少经过站点

## 已包含线路

项目内置多条上海轨道交通线路示例，包括 1 号线至 18 号线、市域机场线、磁浮线和金山铁路。

> 线路和站点数据主要用于课程设计演示，不保证与实时运营信息完全一致。

## 技术栈

- C++17
- Qt Widgets
- CMake
- 邻接表
- Dijkstra 算法

## 数据结构设计

地铁网络使用图结构表示：

- 站点：保存站名、坐标、所属线路
- 边：保存相邻站点和所属线路
- 图：使用邻接表存储站点连接关系

```cpp
QMap<QString, QVector<Edge>> adj;
```

路径搜索时，将“当前站点 + 当前线路”作为状态，使用改进 Dijkstra 算法计算路线。

## 运行方法

使用 Qt Creator 打开项目中的 `CMakeLists.txt`，配置 Qt Kit 后直接运行。

也可以使用命令行构建：

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## CMake 示例

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

## 使用说明

1. 启动程序
2. 选择起点站和终点站
3. 点击“生成换乘指南并高亮路径”
4. 查看换乘方案和图中高亮路线

## 注意事项

本项目仅用于课程学习和算法演示。如果项目中使用了第三方地铁线路图，请在公开发布前确认图片版权或替换为自制示意图。


## 作者的话

真的力竭了。 

之前ai建议用qt做，我嫌麻烦，最后先试了试EasyX。几次连接器和库位置都出了问题，最后才发现根本原因在于Visual Studio由Enterprise换成了Community。但是EasyX还是太恶心了，得不断调坐标，渲染还特别垃圾，最后放弃了，发现Qt Widgets夯爆了。线路图我是用了地铁通的数据捏了一个，虽然还是丑的要命而且存在名称重合等问题，但比高德好太多了（哈哈）。

其实我之前EasyX版本还写了线路首发车、发车频率、换乘时间，根据这个能计算出来怎样最快到目的地的，但是目前这个版本已经很好了，就先交上吧，以后再改。
