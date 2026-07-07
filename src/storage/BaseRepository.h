#pragma once

#include "storage/Database.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVector>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QDebug>

#include <optional>
#include <type_traits>

namespace fininsight::storage {

/**
 * @brief 通用 Repository 模板 — 为 T 提供标准 CRUD 操作
 *
 * T 必须是可默认构造的 struct/class。
 *
 * 使用方式：
 *   class StockRepository : public BaseRepository<Stock> {
 *   protected:
 *       Stock fromSqlQuery(const QSqlQuery& q) const override { ... }
 *       void  bindInsert(QSqlQuery& q, const Stock& s) const override { ... }
 *       void  bindUpdate(QSqlQuery& q, const Stock& s) const override { ... }
 *   };
 *
 *   StockRepository repo(Database::instance());
 *   repo.insert(Stock{"AAPL", "Apple Inc.", ...});
 *   auto maybe = repo.findBySymbol("AAPL");
 */
template <typename T>
class BaseRepository {
public:
    explicit BaseRepository(Database& db, const QString& tableName)
        : db_(db), table_(tableName) {}

    virtual ~BaseRepository() = default;

    // ── 查询 ──────────────────────────────────────────

    /// 按主键 id 查找
    std::optional<T> findById(int id) const {
        return one("SELECT * FROM " + table_ + " WHERE id = ?", {id});
    }

    /// 查找所有记录
    QVector<T> findAll() const {
        return many("SELECT * FROM " + table_, {});
    }

    /// 带条件查询多条记录
    QVector<T> where(const QString& condition, const QVariantList& params = {}) const {
        return many("SELECT * FROM " + table_ + " WHERE " + condition, params);
    }

    /// 计数
    int count() const {
        QSqlQuery q = db_.query();
        if (q.exec("SELECT COUNT(*) FROM " + table_) && q.next()) {
            return q.value(0).toInt();
        }
        return 0;
    }

    /// 是否存在
    bool exists(int id) const {
        return findById(id).has_value();
    }

    // ── 修改 ──────────────────────────────────────────

    /// 插入一条记录，返回自增 id
    bool insert(const T& entity, int* outId = nullptr) {
        // 构造 INSERT 列名
        QStringList cols = columns();
        QString placeholders;
        for (int i = 0; i < cols.size(); ++i) {
            placeholders += (i > 0 ? ", " : "") + QString("?");
        }

        QSqlQuery q = db_.query();
        q.prepare(QString("INSERT INTO %1 (%2) VALUES (%3)")
                      .arg(table_, cols.join(", "), placeholders));

        bindInsert(q, entity);

        if (!q.exec()) {
            qWarning() << "[BaseRepository] INSERT failed:"
                       << q.lastError().text();
            return false;
        }

        if (outId) {
            *outId = q.lastInsertId().toInt();
        }
        return true;
    }

    /// 更新一条记录（entity 必须包含 id 字段）
    bool update(const T& entity) {
        QStringList cols = columns();
        QString sets;
        for (int i = 0; i < cols.size(); ++i) {
            sets += (i > 0 ? ", " : "") + cols[i] + " = ?";
        }

        QSqlQuery q = db_.query();
        q.prepare(QString("UPDATE %1 SET %2 WHERE id = ?")
                      .arg(table_, sets));

        bindUpdate(q, entity);
        q.addBindValue(/*id 作为最后一个参数*/);

        if (!q.exec()) {
            qWarning() << "[BaseRepository] UPDATE failed:"
                       << q.lastError().text();
            return false;
        }
        return true;
    }

    /// 按 id 删除
    bool remove(int id) {
        QSqlQuery q = db_.query();
        q.prepare("DELETE FROM " + table_ + " WHERE id = ?");
        q.addBindValue(id);
        return q.exec();
    }

protected:
    // ── 子类必须重写的纯虚函数 ────────────────────────

    /// 从 QSqlQuery 当前行构建实体对象
    virtual T fromSqlQuery(const QSqlQuery& query) const = 0;

    /// 绑定 INSERT 参数（按列顺序绑定，不需要绑 id）
    virtual void bindInsert(QSqlQuery& query, const T& entity) const = 0;

    /// 绑定 UPDATE 参数（按列顺序绑定，不需要绑 id，id 由框架追加）
    virtual void bindUpdate(QSqlQuery& query, const T& entity) const = 0;

    /// 返回实体对应的列名列表（不含 id，按 insert/update 顺序）
    virtual QStringList columns() const = 0;

    // ── 内部辅助 ─────────────────────────────────────

    Database& db() const { return db_; }
    const QString& tableName() const { return table_; }

private:
    std::optional<T> one(const QString& sql, const QVariantList& params) const {
        QSqlQuery q = db_.query();
        q.prepare(sql);
        for (int i = 0; i < params.size(); ++i) {
            q.bindValue(i, params[i]);
        }
        if (q.exec() && q.next()) {
            return fromSqlQuery(q);
        }
        return std::nullopt;
    }

    QVector<T> many(const QString& sql, const QVariantList& params) const {
        QVector<T> results;
        QSqlQuery q = db_.query();
        q.prepare(sql);
        for (int i = 0; i < params.size(); ++i) {
            q.bindValue(i, params[i]);
        }
        if (q.exec()) {
            while (q.next()) {
                results.append(fromSqlQuery(q));
            }
        }
        return results;
    }

    Database& db_;      // 数据库引用
    QString table_;     // 表名
};

} // namespace fininsight::storage
