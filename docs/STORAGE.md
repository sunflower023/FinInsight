# Storage 模块文档

> 数据持久化层：SQLite 管理 + Repository 模板 + 版本化迁移

---

## 一、架构总览

```
src/storage/
├── Migration.h              ← 迁移框架结构体定义
├── Database.h / .cpp        ← 数据库管理器（单例）
├── BaseRepository.h         ← 通用 CRUD 模板基类
├── StockRepository.h / .cpp ← 股票数据仓储
└── migrations/
    └── V001_Initial.h       ← v1 建表脚本
```

---

## 二、类关系图

```mermaid
classDiagram
    class Database {
        +instance() Database&
        +open(path) bool
        +close()
        +connection() QSqlDatabase
        +query() QSqlQuery
        +execute(sql, params) bool
        +beginTransaction() bool
        +commit() bool
        -applyMigrations()
        -QMutex mutex_
    }

    class BaseRepository~T~ {
        +findById(id) optional~T~
        +findAll() QVector~T~
        +where(cond, params) QVector~T~
        +insert(entity) bool
        +update(entity) bool
        +remove(id) bool
        #fromSqlQuery(query) T*
        #bindInsert(query, entity)*
        #bindUpdate(query, entity)*
        #columns() QStringList*
        #entityId(entity) int*
    }

    class StockRepository {
        +findBySymbol(sym) optional~Stock~
        +updatePrice(sym, price) bool
    }

    class Migration {
        +int version
        +const char* description
        +function~void(QSqlDatabase)~ up
    }

    class V001_Initial {
        +up(db) void$
    }

    Database --> Migration : 持有列表
    Migration <.. V001_Initial : 回调注册
    BaseRepository --> Database : 依赖注入
    StockRepository --|> BaseRepository
    Stock .. StockRepository : 实体
```

---

## 三、启动流程

```mermaid
sequenceDiagram
    participant main as main.cpp
    participant DB as Database
    participant MR as MigrationRunner
    participant V1 as V001_Initial
    participant SQLite as fininsight.db

    main->>DB: open(path)
    DB->>DB: 创建目录
    DB->>SQLite: QSqlDatabase("QSQLITE")
    DB->>SQLite: PRAGMA journal_mode=WAL
    DB->>SQLite: PRAGMA cache_size=-8000
    DB->>DB: applyMigrations()
    DB->>SQLite: SELECT MAX(version)
    SQLite-->>DB: 0 (首次)
    DB->>MR: 遍历 [{1, V001}]
    DB->>DB: beginTransaction()
    DB->>V1: up(connection)
    V1->>SQLite: CREATE TABLE stocks
    V1->>SQLite: CREATE TABLE klines
    V1->>SQLite: CREATE TABLE watchlist
    V1->>SQLite: CREATE TABLE schema_version
    DB->>SQLite: INSERT schema_version=1
    DB->>DB: commit()
    DB-->>main: true
```

---

## 四、数据库表结构

```mermaid
erDiagram
    stocks {
        int id PK
        string symbol UK
        string name
        string exchange
        string currency
        float last_price
        int updated_at
    }
    klines {
        int id PK
        string symbol FK
        string date
        float open
        float high
        float low
        float close
        int volume
    }
    watchlist {
        string symbol PK
        int added_at
        string note
    }
    schema_version {
        int version PK
        int applied_at
    }
    stocks ||--o{ klines : "1:N 按symbol"
    stocks ||--o{ watchlist : "1:1 按symbol"
```

---

## 五、CRUD 调用示例

```cpp
#include "storage/Database.h"
#include "storage/StockRepository.h"

using namespace fininsight::storage;

// 1. 打开数据库（main.cpp 中已做）
Database::instance().open("path/to/fininsight.db");

// 2. 创建仓储
StockRepository repo(Database::instance());

// 3. 写入
Stock apple{"AAPL", "Apple Inc.", "NASDAQ", "USD", 187.5, time(nullptr)};
repo.insert(&apple);

// 4. 查询
auto maybe = repo.findBySymbol("AAPL");
if (maybe) {
    qDebug() << maybe->name << maybe->lastPrice;
}

// 5. 更新价格
repo.updatePrice("AAPL", 188.0);

// 6. 查询全部
auto all = repo.findAll();
qDebug() << all.size() << "stocks in database";
```

---

## 六、线程安全说明

```cpp
// 主线程：直接使用主连接
Database::instance().query();

// 子线程：自动克隆连接
void workerThread() {
    QSqlDatabase conn = Database::instance().connection();
    // conn 是一个新的数据库连接，指向同一个 .db 文件
    QSqlQuery q(conn);
    q.exec("SELECT * FROM stocks");
}
```

**原理：** 主连接在 open() 时创建，子线程调用 `connection()` 时通过 `QSqlDatabase::cloneDatabase()` 复制一个新连接。SQLite WAL 模式支持一写多读并发。

---

## 七、添加新表

```cpp
// 1. 新建 V002_Portfolio.h
namespace V002_Portfolio {
inline void up(QSqlDatabase db) {
    QSqlQuery q(db);
    q.exec("CREATE TABLE IF NOT EXISTS portfolio (...)");
}
}

// 2. Database.cpp 中 register：
{2, "Add portfolio", V002_Portfolio::up},

// 3. 创建 PortfolioRepository : BaseRepository<Portfolio>
// 重启程序自动迁移，老数据不受影响。
```
