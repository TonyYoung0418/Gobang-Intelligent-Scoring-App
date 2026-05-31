# 五子棋智能评分 App

这是一个基于 Android 客户端和 C++ 服务端引擎的五子棋项目。客户端负责棋盘显示、玩家交互、局面保存与服务器通信；服务端负责智能落子、局面评分和搜索决策。

项目中包含 Android App 源码、界面资源、C++ 服务器源码，以及评分引擎运行所需的数据文件。

## 功能简介

- Android 五子棋对弈界面
- 玩家与电脑对战
- 基于服务器的 AI 落子
- 棋盘局面保存、加载与分享
- 悔棋、结果显示等基础功能
- C++ 实现的棋型评分与搜索引擎
- 支持普通、防守、进攻等不同评分策略

## 项目结构

```text
.
├── Gobang/                         # Android 客户端工程
│   ├── app/src/main/java/          # Java 源码
│   ├── app/src/main/res/           # 布局、图片、音频等资源
│   └── build.gradle                # Android Gradle 配置
│
└── 源代码/
    ├── Gobang/                     # Android 工程备份/源码副本
    └── 服务器/
        ├── 五子棋服务器.sln        # Visual Studio 解决方案
        └── 五子棋服务器/
            ├── 头文件/             # C++ 头文件
            ├── 源文件/             # C++ 源文件
            ├── database.db         # 搜索/数据库文件
            ├── score.db            # 评分相关数据
            └── weight.db           # 棋型权重数据
```

## Android 客户端

主要 Android 工程位于：

```text
Gobang/
```

可以使用 Android Studio 打开该目录。

主要文件说明：

- `Gobang/app/src/main/java/com/example/gobang/Graph.java`  
  棋盘视图核心类，负责棋盘绘制、触摸落子、局面序列化、服务器请求、悔棋和结果显示。

- `Gobang/app/src/main/java/com/example/gobang/MainActivity.java`  
  App 主界面入口。

- `Gobang/app/src/main/java/com/example/gobang/SetServer.java`  
  服务器地址配置。

与电脑对战前，需要先运行 C++ 服务器，并在 App 中配置正确的服务器 IP 地址。

## C++ 服务端

服务端工程位于：

```text
源代码/服务器/
```

可以使用 Visual Studio 打开：

```text
源代码/服务器/五子棋服务器.sln
```

服务端源码主要分布在：

- `源代码/服务器/五子棋服务器/头文件/`
- `源代码/服务器/五子棋服务器/源文件/`

主要文件说明：

- `graph.h` / `graph.cpp`  
  棋盘模型、落子/撤销、候选点维护、局面分数的增量更新。

- `constant.h` / `constant.cpp`  
  评分常量、棋型等级、预处理评分表和权重数据加载。

- `engine.cpp`  
  AI 搜索与落子选择逻辑。

- `server.h`  
  HTTP 服务接口处理。

服务端运行时依赖以下数据文件：

- `database.db`
- `score.db`
- `weight.db`

运行服务器时，需要确保这些文件位于程序能够读取到的工作目录中。

## 局面评分算法简介

评分引擎的核心思路是：

- 使用 `weight.db` 中的棋型权重为局面打基础分
- 分别计算横、竖、两条斜线四个方向的分数
- 对多方向威胁进行额外加成
- 分别维护电脑和玩家的候选点分数
- 根据普通、防守或进攻策略组合成最终局面分

单个候选点的大致评分方式：

```text
点位分数 = 横向分
        + 纵向分
        + 左上-右下斜线分
        + 右上-左下斜线分
        + 多方向威胁加成
```

全局局面分大致为：

```text
普通策略：电脑总分 - 玩家总分
防守策略：电脑总分 - 2 * 玩家总分
进攻策略：2 * 电脑总分 - 玩家总分
```

为了提高搜索效率，服务端不会每一步都重新扫描整张棋盘，而是在落子后只更新受影响范围内的空位评分。

## 说明

- 仓库中保留源码、资源文件和评分引擎所需数据文件。
- 构建产物、IDE 缓存和本机配置不会提交。
- 已忽略 `build/`、`.gradle/`、`.idea/`、`.vs/`、`local.properties` 等文件或目录。

## License

暂未指定开源协议。
