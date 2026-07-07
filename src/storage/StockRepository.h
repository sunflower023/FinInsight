#pragma once

#include "storage/BaseRepository.h"

#include <QString>
#include <optional>

namespace fininsight::storage {

/**
 * @brief 股票实体 — 对应 stocks 表
 */
struct Stock {
    int     id        = 0;
    QString symbol;          // "AAPL" / "600519"
    QString name;            // "Apple Inc." / "贵州茅台"
    QString exchange;        // "NASDAQ" / "SSE"
    QString currency;        // "USD" / "CNY"
    double  lastPrice = 0.0;
    qint64  updatedAt = 0;   // Unix timestamp
};

/**
 * @brief 股票数据访问层
 *
 * 继承自 BaseRepository<Stock>，额外提供了按 symbol 查询。
 */
class StockRepository : public BaseRepository<Stock> {
public:
    explicit StockRepository(Database& db);

    /// 按股票代码查询
    std::optional<Stock> findBySymbol(const QString& symbol) const;

    /// 更新最新价格和更新时间
    bool updatePrice(const QString& symbol, double price);

protected:
    Stock   fromSqlQuery(const QSqlQuery& q)   const override;
    void    bindInsert(QSqlQuery& q, const Stock& s) const override;
    void    bindUpdate(QSqlQuery& q, const Stock& s) const override;
    QStringList columns() const override;
};

} // namespace fininsight::storage
