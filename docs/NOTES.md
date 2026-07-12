# 开发笔记

> 项目进行中遇到的问题与解答，按模块分类。

---

## storage 模块

### Q1: databasePath / cachePath / dataPath 分别存什么？

```
dataPath     → %LOCALAPPDATA%/FinInsight/           根目录
databasePath → .../fininsight.db                    stocks/klines/watchlist 表
cachePath    → .../cache/                           HTTP响应缓存、临时文件
```

数据库存结构化持久数据（股票代码、K线、自选股），缓存存可丢弃的临时数据。分目录是为了方便用户清缓存而不影响自选股。

### Q2: 这个单例写法线程安全吗？为什么不是双重检查锁定？

**安全。** 用的是 C++11 Meyer's Singleton：

```cpp
static Database& instance() {
    static Database db;    // C++11 保证线程安全初始化
    return db;
}
```

C++11 标准规定函数内 static 变量初始化是线程安全的（编译器插入原子标志位）。双重检查锁定是 C++11 之前的写法，现在没必要。Meyer's Singleton 更简洁、无锁、自动析构。

### Q3: SQLite 的 PRAGMA 缓存优化有必要吗？

**有必要。** SQLite 的 `cache_size` = 页缓存（存储引擎层），不是 MySQL 8.0 废弃的 Query Cache（结果集缓存）。

| PRAGMA | 作用 | 不开的后果 |
|--------|------|-----------|
| `journal_mode=WAL` | 一写多读并发 | 读写互斥阻塞 |
| `synchronous=NORMAL` | 减少 fsync 次数 | 写入慢 10-50 倍 |
| `cache_size=-8000` | 8MB B-Tree 页缓存 | 频繁磁盘IO |

### Q4: 为什么子线程要 clone 新连接？

**SQLite 限制：** QSqlDatabase 不是线程安全的。Qt 文档规定连接只能在创建它的线程中使用。多线程必须 clone：

```cpp
// 主线程 → 复用主连接
// 子线程 → cloneDatabase() 创建新连接指向同一个 .db 文件
```

SQLite WAL 模式允许多个连接同时操作同一文件（一写多读），但不能共用一个 QSqlDatabase 对象。

### Q5: 有 SQL 注入风险吗？

**没有。** 全部使用参数化查询（PreparedStatement 等价方案）：

```cpp
// ✅ 安全 — 当前全部写法
q.prepare("SELECT * FROM stocks WHERE symbol = ?");
q.addBindValue(userInput);   // Qt 自动转义

// ❌ 危险 — 代码中没有
q.exec("SELECT * FROM stocks WHERE symbol = '" + userInput + "'");
```

唯一风险点：表名/列名是硬编码的（如 `"stocks"`），不是用户输入，安全。将来若允许用户指定，需加白名单。

### Q6: Migration 迁移框架是干什么的？

**让数据库表结构随时间演进，不用删库重建。** 比如 v1 有 3 张表，v2 要加 `portfolio` 表，启动时只跑 v2 的 SQL，老数据不动。

**三个文件协作：**

```
Migration.h         → 定义结构体 {版本号, 描述, 回调函数}
V001_Initial.h      → 具体迁移：v1 建表 SQL（4张表）
Database.cpp        → 引擎：读 schema_version 表 → 跳过已执行 → 跑缺失的
```

**流程：**

```
启动 → Database::open() → applyMigrations()
  → 读当前版本号（schema_version 表）
  → 遍历 [{1, V001}, {2, V002}, ...]
  → version > current → 执行 up() → 更新版本号
  → version ≤ current → 跳过
```

**加新表示例：**

```cpp
// 1. 新建 V002_Portfolio.h — 写 CREATE TABLE SQL
// 2. Database.cpp 加一行：
{2, "Add portfolio table", V002_Portfolio::up},
// 程序重启自动跑，老数据不受影响。
```

---

## 单例模式

### 本项目用到的单例

| 类 | 作用 |
|----|------|
| `AppConfig` | 全局配置（路径管理） |
| `Database` | 数据库连接管理 |

### 模板方法模式

| 基类 | 定义流程 | 子类实现 |
|------|---------|---------|
| `BaseRepository<T>` | insert/update/find 流程 | fromSqlQuery / bindInsert / bindUpdate / columns |

---

## 构建相关

| 问题 | 解决 |
|------|------|
| CMake 找不到 Qt6 | 检查 `CMAKE_PREFIX_PATH` 指向 Qt 安装路径 |
| FetchContent 下载失败 | 配 Git 代理或 Clash TUN 模式 |
| 编译完双击 exe 没反应 | 缺 Qt DLL，运行 `windeployqt` 或在 CMakeLists 中配置 POST_BUILD |
| windeployqt 路径 | `D:\Qt\6.7.3\msvc2022_64\bin\windeployqt.exe` |

---

> 最后更新: 2026-07-13
