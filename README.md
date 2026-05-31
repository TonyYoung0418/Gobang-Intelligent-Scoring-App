# 五子棋智能评分 App

这是一个 Android + C++ 服务端组成的五子棋智能评分系统。Android 端负责棋盘交互、界面展示、局面序列化、网络请求和配置管理；C++ 服务端负责解析棋盘、判断胜负、计算电脑落子，并通过棋型评分与搜索算法给出当前局面的评分。

项目包含 Android 客户端源码、C++ 服务端源码、界面资源，以及服务端评分引擎运行所需的数据文件。

## 核心功能

### Android 客户端

- 15 x 15 五子棋棋盘绘制
- 玩家点击棋盘落子
- 电脑自动应手
- 新游戏
- 悔棋
- 智能提示落子位置
- 显示当前棋局评分
- 显示胜、负、平结果
- 设置玩家是否先手
- 设置电脑难度
- 设置服务器 IP 地址
- 登录、注册、记住密码、自动登录
- 棋局二维码生成与识别代码
- 棋盘状态序列化与恢复
- 网络异常提示
- 电脑计算时显示等待状态
- 落子音效与棋子图片显示

说明：`Share.java` 中已经实现二维码生成、扫码和相册识别逻辑；主界面菜单中的分享入口目前在 `MainActivity.java` 中处于注释状态。

### C++ 服务端

- 基于 HTTP 的 `/server` 接口
- 接收 Android 端发送的棋盘 JSON
- 将客户端棋盘映射为服务端棋盘模型
- 判断玩家是否已经五连
- 根据难度搜索电脑落子
- 返回电脑落子坐标
- 返回胜负状态
- 返回局面评分
- 支持平局判断
- 通过互斥锁保证请求处理过程中的棋盘状态一致

## 项目结构

```text
.
├── Gobang/                         # Android 客户端工程
│   ├── app/src/main/java/          # Java 源码
│   ├── app/src/main/res/           # 布局、图片、音频等资源
│   ├── app/src/main/assets/        # 字体资源
│   └── build.gradle                # Android Gradle 配置
│
└── 源代码/
    ├── Gobang/                     # Android 工程备份/源码副本
    └── 服务器/
        ├── 五子棋服务器.sln        # Visual Studio 解决方案
        └── 五子棋服务器/
            ├── 头文件/             # C++ 头文件
            ├── 源文件/             # C++ 源文件
            ├── database.db         # 搜索/开局相关数据
            ├── score.db            # 评分相关数据
            └── weight.db           # 棋型权重数据
```

## Android 端实现

### 主界面

主界面入口为：

```text
Gobang/app/src/main/java/com/example/gobang/MainActivity.java
```

主要负责：

- 初始化标题栏和图标
- 绑定棋盘视图 `Graph`
- 绑定棋局评分文本
- 绑定等待进度条
- 处理“新游戏”“悔棋”“提示”按钮
- 打开服务器设置页面
- 打开游戏设置页面
- 打开关于页面
- 接收设置页返回的数据并更新棋盘配置

### 棋盘视图

棋盘核心类为：

```text
Gobang/app/src/main/java/com/example/gobang/Graph.java
```

`Graph` 继承自 Android `View`，是客户端最核心的类，主要职责包括：

- 维护 15 x 15 棋盘数组 `graph`
- 使用 `pieces` 保存落子历史
- 根据控件尺寸自适应绘制正方形棋盘
- 绘制棋盘线条、黑白棋子、最新电脑落子标记、提示标记和胜负文字
- 根据触摸坐标寻找最近的棋盘交叉点
- 判断该点是否为空，并执行玩家落子
- 通过 OkHttp 向服务器发送局面
- 接收服务器返回的电脑落子、结果和评分
- 在等待服务器响应时禁止重复操作
- 处理连接失败时的回滚
- 支持悔棋，默认撤销玩家和电脑最近两步
- 支持局面编码 `dump()` 和恢复 `load()`

客户端棋子含义：

```text
EMPTY = -1
BLACK = 0  // 电脑玩家
WHITE = 1  // 人类玩家
```

结果状态：

```text
LOSE    = 0  // 玩家失败
WIN     = 1  // 玩家胜利
TIE     = 2  // 平局
UNKNOWN = 3  // 未结束
```

### 与服务器通信

普通电脑落子请求由 `postMessage()` 发起，请求地址为：

```text
http://{ip}:8888/server
```

请求体使用表单字段 `json` 携带 JSON 字符串，主要字段如下：

```json
{
  "machine": 0,
  "person": 1,
  "difficulty": 10,
  "board": [-1, -1, 0, 1]
}
```

字段含义：

- `machine`：客户端中电脑棋子的编号
- `person`：客户端中玩家棋子的编号
- `difficulty`：搜索难度
- `board`：长度为 225 的一维棋盘数组，按行展开

电脑落子时，客户端发送：

```text
difficulty = 6 + diff * 2
```

其中 `diff` 来自设置页，默认有低、中、高三档。

提示功能由 `hint()` 发起。为了让服务器站在“玩家视角”计算推荐点，提示请求会交换 `machine` 和 `person` 的含义：

```text
machine = WHITE
person  = BLACK
difficulty = 10
```

服务器响应示例：

```json
{
  "position": [7, 7],
  "score": 12,
  "result": "unknown"
}
```

可能的 `result`：

- `unknown`：游戏继续
- `win`：玩家胜利
- `lose`：电脑胜利
- `tie`：平局

### 局面保存与二维码

`Graph.dump()` 将棋盘压缩为字符串：

- 每个棋盘格先从 `-1/0/1` 转为 `0/1/2`
- 每两个格子合并成一个 0 到 8 的数字
- 追加当前结果状态
- 追加当前局面评分

`Graph.load()` 执行反向解析，将字符串恢复为棋盘、结果和评分。

`Share.java` 中使用 ZXing：

- 生成带 App 图标的二维码
- 调用相机扫码
- 从相册选择图片解析二维码
- 将解析到的字符串交回主界面恢复棋局

### 设置与登录

`Config.java` 使用 `SharedPreferences` 保存：

- 是否玩家先手
- 电脑难度档位

`SetServer.java` 使用 `SharedPreferences` 保存服务器 IP，并校验 IP 地址格式。

`LoginActivity.java` 和 `RegisterActivity.java` 实现本地登录注册流程：

- 默认账号密码
- 注册新账号
- 记住密码
- 自动登录
- 登录成功后进入主界面

## 服务端实现

### HTTP 接口

服务端入口逻辑在：

```text
源代码/服务器/五子棋服务器/头文件/server.h
```

服务端监听：

```text
0.0.0.0:8888
```

接口路径：

```text
POST /server
```

处理流程：

1. 从请求参数 `json` 中解析客户端棋盘。
2. 清空服务端棋盘。
3. 遍历 225 个棋盘格，将客户端棋子转换为服务端 `MACHINE_ID` 或 `PERSON_ID`。
4. 调用 `check(PERSON)` 判断玩家是否已经五连。
5. 规范化难度，范围限制在 2 到 10，奇数难度自动加 1。
6. 调用 `great_pos(graph, false, difficulty)` 搜索电脑落子。
7. 将电脑落子写入棋盘。
8. 返回落子坐标、评分和结果。
9. 判断是否平局或电脑胜利。

### 胜负判断

`Server::check(char type)` 会扫描四个方向：

- 竖向
- 横向
- 左上到右下
- 右上到左下

只要某一方向存在连续 5 个同色棋子，就返回这 5 个棋子的位置。

## 评分算法实现

评分相关代码主要在：

```text
源代码/服务器/五子棋服务器/源文件/constant.cpp
源代码/服务器/五子棋服务器/源文件/graph.cpp
源代码/服务器/五子棋服务器/头文件/graph.h
```

### 1. 棋型权重

服务端启动时调用 `init_constant()`，读取：

```text
weight.db
```

读取到的 `WEIGHT[state][wall_type]` 表示某个一维棋型在不同堵塞情况下的基础分。

`wall_type` 分为三类：

```text
empty       // 开放
space_wall  // 隔一格被堵
just_wall   // 紧贴被堵
```

两侧墙型通过：

```cpp
JOIN_WALL_TYPE(left_wall_type, right_wall_type)
```

合并为权重表索引。

### 2. 预处理评分表

五子棋的横、竖、斜线都可以看作一维数组。为了避免搜索时重复分析棋型，程序启动时会预处理：

```cpp
SCORE_TABLE[线长][己方棋子位状态][落子位置]
```

预处理过程会枚举：

- 不同长度的一维线段
- 该线段上的所有己方棋子位状态
- 每一个可落子位置

然后调用 `score(position)` 计算该落点形成的最佳棋型分，并存入 `SCORE_TABLE`。

这样搜索过程中计算某个点的方向分时，只需要通过位运算提取状态，再查表即可。

### 3. 位状态表示

`graph_t` 使用 `line[4][...][2]` 保存棋盘四个方向上双方棋子的位图状态：

```text
direction = 0  横向
direction = 1  纵向
direction = 2  左上-右下
direction = 3  右上-左下
```

每次落子时，会将该位置写入对应方向的位图中。

计算某点在某方向上的分数时：

1. 取出己方在该线上的位图 `st`。
2. 取出对方在该线上的位图 `st2`。
3. 将对方棋子视为边界。
4. 计算该点所在的有效连续空间范围。
5. 从己方位图中截取对应范围。
6. 查 `SCORE_TABLE` 得到方向分。

对应函数：

```cpp
graph_t::score(pos position, int direction, int id)
```

### 4. 单点评分

一个候选点会计算四个方向的分数：

```text
横向分
纵向分
左上-右下斜线分
右上-左下斜线分
```

再加上多方向组合加成：

```cpp
sc1 + sc2 + sc3 + sc4
+ ADDITION4[RANK[sc1]][RANK[sc2]][RANK[sc3]][RANK[sc4]]
```

其中：

- `RANK` 将原始分数映射成 7 个威胁等级
- `ADDITION4` 给双活三、活三加冲四、双冲四等复合威胁额外加分

因此算法不仅看单条线上的棋型，也会提升“多个方向同时形成威胁”的点。

### 5. 候选点维护

每个棋盘点对应一个 `node`，保存：

```cpp
direct_score[2][4]  // 双方在四个方向的分数
score_sum[2]        // 双方在该点的综合分
score_max[2]        // 双方在该点的最大威胁分
```

`score_sum[id]` 表示玩家 `id` 在该点落子的总体价值。

`score_max[id]` 只取最强的两个方向再加组合奖励，用于快速识别强制威胁：

```cpp
max_sc + max_sc2 + ADDITION2[RANK[max_sc]][RANK[max_sc2]]
```

### 6. 增量更新

落子后，程序不会重算全盘。`graph_t::set()` 只更新落子点周围 8 个方向、一定范围内受影响的空点。

更新函数包括：

- `maintain()`
- `maintain_single()`
- `update_max()`
- `adjust_sum()`
- `adjust_max()`

这些函数会更新：

- 某方向分数
- 单点总分
- 最大威胁分
- 全局总分
- 候选点排序链表

这使得 AI 在搜索中反复落子和撤销时仍能保持较高效率。

### 7. 全局局面分

全局维护双方总分：

```cpp
grades[MACHINE_ID]
grades[PERSON_ID]
```

单点分进入全局分时会平方：

```cpp
DUMP_SCORE(sc) = sc * sc
```

平方的作用是放大高价值点，使一个强威胁点比多个普通点更重要。

最终局面分根据策略不同计算：

```text
普通策略：电脑总分 - 玩家总分
防守策略：电脑总分 - 2 * 玩家总分
进攻策略：2 * 电脑总分 - 玩家总分
```

对应代码在 `graph_t::score()`。

### 8. 胜势判断

服务端用若干阈值判断一方是否已经形成强制胜势：

```cpp
SUCCESS_1 = 100000
SUCCESS_2 = 34000
SUCCESS_3 = 24000
```

`graph_t::success()` 会比较己方最大威胁分和对方最大威胁分，判断当前是否存在必胜或接近必胜的局面。

`graph_t::control()` 用来判断某一方是否已经形成必须应对的强威胁。

## 搜索算法实现

搜索核心在：

```text
源代码/服务器/五子棋服务器/源文件/engine.cpp
```

### 1. Alpha-Beta 搜索

`alphabeta()` 是主要搜索函数。搜索过程中：

- 电脑方尝试最大化局面分
- 玩家方尝试最小化局面分
- 达到深度上限时返回 `graph.score()`
- 出现胜势时提前返回极大/极小值
- 使用 alpha-beta 剪枝减少搜索节点

搜索时优先考虑：

1. 已知最优点或当前最高分候选点
2. 排序链表中的高价值候选点
3. 最多扩展 `COLUMN_WIDTH` 个候选点

### 2. 哈希表与重复局面

搜索中使用 `state_t` 压缩当前局面状态，并通过 `hash_table_t` 保存搜索结果。

哈希表保存：

- 推荐落点
- 剩余搜索深度
- 局面估值
- 结果类型：确定值或边界值

如果搜索遇到已经计算过、且深度足够的局面，就可以直接复用结果或更新 alpha/beta 边界。

### 3. 迭代加深

搜索入口 `search_thread_first()` 会从较浅深度开始逐步加深：

```cpp
for (int limit = 2; limit <= max_depth - 2; limit += 2)
```

先得到较浅层的推荐点，再作为深层搜索的优先候选，提高剪枝效率。

### 4. 多线程搜索

`great_pos()` 内部使用多份 `graph`、`state`、`hash_table`，并通过 `std::async` 同时搜索多个候选方向或策略分支。

最终从多个搜索结果中选择评分最高的落点。

### 5. 攻防策略切换

搜索过程中会根据当前评估分调整策略：

- 如果局面对电脑明显不利，切换为防守策略
- 如果局面对电脑明显有利，切换为进攻策略
- 否则保持普通策略

策略会影响 `graph.score()` 中机器分和玩家分的权重，从而改变搜索倾向。

### 6. 数据库辅助

`init_engine()` 会读取：

```text
database.db
```

该数据库用于辅助搜索或开局/局面选择。引擎启动时只初始化一次，避免重复加载。

### 7. 输出评分映射

搜索内部的 `grades` 数值可能很大。返回给 Android 端前，`getScore()` 会将其压缩到更适合 UI 展示的范围：

```cpp
score = pow(abs(grades), 0.19)
score = clamp(score, -100, 100)
return -score
```

最终客户端显示：

```text
棋局评分：score
```

## 运行方式

### 1. 启动服务端

使用 Visual Studio 打开：

```text
源代码/服务器/五子棋服务器.sln
```

编译并运行服务端。服务端默认监听：

```text
0.0.0.0:8888
```

请确保以下文件位于服务端运行时可读取的工作目录中：

```text
database.db
score.db
weight.db
```

### 2. 启动 Android 客户端

使用 Android Studio 打开：

```text
Gobang/
```

运行 App 后，在菜单中进入服务器设置页面，填写服务端 IP 地址。

### 3. 开始对局

进入主界面后：

- 点击棋盘落子
- App 自动请求服务端计算电脑落子
- 点击“提示”可请求推荐位置
- 点击“悔棋”可撤销最近一轮
- 点击“新游戏”可重新开始

## 依赖与技术栈

### Android

- Java
- Android View 自定义绘制
- OkHttp 网络请求
- SharedPreferences 本地存储
- ZXing 二维码生成与识别
- Gradle 构建

### C++ 服务端

- C++
- cpp-httplib HTTP 服务
- nlohmann/json JSON 解析
- Alpha-Beta 搜索
- 哈希表缓存
- 棋型评分表
- 位运算棋盘状态压缩
- 多线程搜索

## 说明

- 仓库中保留源码、资源文件和评分引擎所需数据文件。
- 构建产物、IDE 缓存和本机配置不会提交。
- 已忽略 `build/`、`.gradle/`、`.idea/`、`.vs/`、`local.properties` 等文件或目录。
- 当前仓库未提交编译后的可执行文件或 APK。

## License

本项目使用 MIT License 开源，详见 [LICENSE](LICENSE)。
