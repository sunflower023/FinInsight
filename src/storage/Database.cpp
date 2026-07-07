#include "storage/Database.h"
#include "storage/Migration.h"
#include "storage/migrations/V001_Initial.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QSqlQuery>
#include <QThread>
#include <QDebug>

namespace fininsight::storage {

// ───────────────────────────────────────────────────────────────
//  单例
// ───────────────────────────────────────────────────────────────

Database& Database::instance() {
    static Database db;
    return db;
}

Database::~Database() {
    close();
}

// ───────────────────────────────────────────────────────────────
//  打开/关闭
// ───────────────────────────────────────────────────────────────

bool Database::open(const QString& path) {
    QMutexLocker locker(&mutex_);

    if (isOpen()) {
        qWarning() << "[Database] Already open at:" << dbPath_;
        return true;
    }

    // 确保目录存在
    QDir().mkpath(QFileInfo(path).absolutePath());

    // 创建唯一连接名
    connName_ = QString("fininsight_main_%1").arg(
        reinterpret_cast<quintptr>(this), 0, 16);

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName_);
        db.setDatabaseName(path);

        if (!db.open()) {
            qCritical() << "[Database] Failed to open:" << path
                        << "Error:" << db.lastError().text();
            return false;
        }

        dbPath_ = path;
    }

    // SQLite 性能优化：WAL 模式 + 适当缓存
    execute("PRAGMA journal_mode=WAL");
    execute("PRAGMA synchronous=NORMAL");
    execute("PRAGMA cache_size=-8000");      // 8MB 缓存
    execute("PRAGMA foreign_keys=ON");
    execute("PRAGMA busy_timeout=5000");     // 锁等待 5 秒

    qInfo() << "[Database] Opened successfully:" << path;
    applyMigrations();
    return true;
}

void Database::close() {
    QMutexLocker locker(&mutex_);

    if (!isOpen()) return;

    QSqlDatabase::database(connName_).close();
    QSqlDatabase::removeDatabase(connName_);

    QString closed = dbPath_;
    dbPath_.clear();
    connName_.clear();

    qInfo() << "[Database] Closed:" << closed;
}

// ───────────────────────────────────────────────────────────────
//  连接获取
// ───────────────────────────────────────────────────────────────

QSqlDatabase Database::connection() {
    // 主线程直接复用主连接
    if (QThread::currentThread() == QApplication::instance()->thread()) {
        return mainConnection();
    }

    // 子线程：按线程名创建克隆连接
    QMutexLocker locker(&mutex_);
    const QString name = QString("fininsight_th_%1")
        .arg(reinterpret_cast<quintptr>(QThread::currentThread()), 0, 16);

    QSqlDatabase conn = QSqlDatabase::cloneDatabase(mainConnection(), name);
    if (!conn.open()) {
        qWarning() << "[Database] clone failed:" << conn.lastError().text();
    }
    return conn;
}

QSqlDatabase Database::mainConnection() {
    return QSqlDatabase::database(connName_);
}

// ───────────────────────────────────────────────────────────────
//  便捷查询
// ───────────────────────────────────────────────────────────────

QSqlQuery Database::query() {
    return QSqlQuery(mainConnection());
}

bool Database::execute(const QString& sql, const QVariantList& params) {
    QSqlQuery q = query();
    q.prepare(sql);
    for (int i = 0; i < params.size(); ++i) {
        q.bindValue(i, params[i]);
    }
    if (!q.exec()) {
        qWarning() << "[Database] Execute failed:" << q.lastError().text()
                   << "\n  SQL:" << sql.left(200);
        return false;
    }
    return true;
}

// ───────────────────────────────────────────────────────────────
//  事务
// ───────────────────────────────────────────────────────────────

bool Database::beginTransaction() {
    return mainConnection().transaction();
}

bool Database::commit() {
    return mainConnection().commit();
}

bool Database::rollback() {
    return mainConnection().rollback();
}

// ───────────────────────────────────────────────────────────────
//  状态
// ───────────────────────────────────────────────────────────────

bool Database::isOpen() const {
    return !connName_.isEmpty()
        && QSqlDatabase::database(connName_).isOpen();
}

QString Database::lastError() const {
    return QSqlDatabase::database(connName_).lastError().text();
}

// ───────────────────────────────────────────────────────────────
//  迁移框架
// ───────────────────────────────────────────────────────────────

void Database::applyMigrations() {
    using namespace migrations;

    // 构建迁移列表（新增版本只需往下加一行）
    QVector<Migration> migrations = {
        {1, "Initial schema (stocks, klines, watchlist)", V001_Initial::up},
        // {2, "Add portfolio table", V002_Portfolio::up},  ← 未来扩展
    };

    // 创建 schema_version 表（如果不存在）
    {
        QSqlQuery q = query();
        q.exec(R"(
            CREATE TABLE IF NOT EXISTS schema_version (
                version    INTEGER PRIMARY KEY,
                applied_at INTEGER DEFAULT (strftime('%s','now'))
            )
        )");
    }

    // 读取当前版本
    int currentVersion = 0;
    {
        QSqlQuery q = query();
        if (q.exec("SELECT MAX(version) FROM schema_version") && q.next()) {
            currentVersion = q.value(0).toInt();
        }
    }

    qInfo() << "[Database] Current schema version:" << currentVersion
            << "| Pending:" << (migrations.size() - currentVersion);

    // 执行待处理的迁移
    for (const auto& m : migrations) {
        if (m.version <= currentVersion) continue;

        qInfo() << "[Database] Applying migration v" << m.version
                << "-" << m.description;

        beginTransaction();
        m.up(mainConnection());
        execute("INSERT OR REPLACE INTO schema_version (version) VALUES (?)",
                {m.version});
        commit();
    }
}

} // namespace fininsight::storage
