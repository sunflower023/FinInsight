#include "storage/StockRepository.h"

#include <QSqlQuery>

namespace fininsight::storage {

// ───────────────────────────────────────────────────────────────
//  构造
// ───────────────────────────────────────────────────────────────

StockRepository::StockRepository(Database& db)
    : BaseRepository<Stock>(db, "stocks")
{}

// ───────────────────────────────────────────────────────────────
//  附加查询
// ───────────────────────────────────────────────────────────────

std::optional<Stock> StockRepository::findBySymbol(const QString& symbol) const {
    auto results = where("symbol = ?", {symbol});
    if (results.isEmpty()) return std::nullopt;
    return results.first();
}

bool StockRepository::updatePrice(const QString& symbol, double price) {
    QSqlQuery q = db().query();
    q.prepare("UPDATE stocks SET last_price = ?, updated_at = strftime('%s','now') WHERE symbol = ?");
    q.addBindValue(price);
    q.addBindValue(symbol);
    return q.exec();
}

// ───────────────────────────────────────────────────────────────
//  子类必须实现的方法
// ───────────────────────────────────────────────────────────────

Stock StockRepository::fromSqlQuery(const QSqlQuery& q) const {
    Stock s;
    s.id        = q.value("id").toInt();
    s.symbol    = q.value("symbol").toString();
    s.name      = q.value("name").toString();
    s.exchange  = q.value("exchange").toString();
    s.currency  = q.value("currency").toString();
    s.lastPrice = q.value("last_price").toDouble();
    s.updatedAt = q.value("updated_at").toLongLong();
    return s;
}

void StockRepository::bindInsert(QSqlQuery& q, const Stock& s) const {
    // 注意：不绑定 id（自增），顺序要和 columns() 一致
    q.addBindValue(s.symbol);
    q.addBindValue(s.name);
    q.addBindValue(s.exchange);
    q.addBindValue(s.currency);
    q.addBindValue(s.lastPrice);
    q.addBindValue(s.updatedAt);
}

void StockRepository::bindUpdate(QSqlQuery& q, const Stock& s) const {
    // SET 子句按 columns() 顺序绑定（id 由基类追加）
    q.addBindValue(s.symbol);
    q.addBindValue(s.name);
    q.addBindValue(s.exchange);
    q.addBindValue(s.currency);
    q.addBindValue(s.lastPrice);
    q.addBindValue(s.updatedAt);
}

QStringList StockRepository::columns() const {
    return {"symbol", "name", "exchange", "currency", "last_price", "updated_at"};
}

} // namespace fininsight::storage
