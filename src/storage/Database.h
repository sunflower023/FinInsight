#pragma once

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QMutex>

#include <functional>
#include <memory>

namespace fininsight::storage {

/**
 * @brief 数据库连接管理器（线程安全单例）
 *
 * 职责：
 *   - 管理 SQLite 连接（WAL 模式，一写多读）
 *   - 主线程持有主连接，子线程通过 connection() 获取克隆连接
 *   - 自动运行版本化数据库迁移
 *
 * 使用：
 *   Database::instance().open("/path/to/db.sqlite");
 *   QSqlQuery q = Database::instance().query();
 *   q.exec("SELECT * FROM stocks");
 */
class Database {
public:
    static Database& instance();

    // —— 生命周期 ——
    bool open(const QString& path);
    void close();

    // —— 连接获取 ——
    QSqlDatabase connection();       // 获取当前线程可用的连接
    QSqlDatabase mainConnection();   // 主线程连接

    // —— 便捷查询 ——
    QSqlQuery query();             // 在主连接上创建 QSqlQuery
    bool execute(const QString& sql, const QVariantList& params = {});

    // —— 事务 ——
    bool beginTransaction();
    bool commit();
    bool rollback();

    // —— 状态 ——
    bool isOpen() const;
    QString lastError() const;
    QString path() const { return dbPath_; }

private:
    Database() = default;
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    void applyMigrations();

    QString dbPath_;
    QString connName_;              // 主连接名
    mutable QMutex mutex_;
};

} // namespace fininsight::storage
