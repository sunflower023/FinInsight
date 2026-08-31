#pragma once
#include <QSqlDatabase>
#include <QSqlQuery>
namespace fininsight::storage::migrations::V004_Trading {
inline void up(QSqlDatabase db) {
    QSqlQuery q(db);
    q.exec(R"(CREATE TABLE IF NOT EXISTS trading_orders (
        client_order_id TEXT PRIMARY KEY, broker_order_id TEXT NOT NULL DEFAULT '', environment TEXT NOT NULL,
        symbol TEXT NOT NULL, side INTEGER NOT NULL, order_type INTEGER NOT NULL, quantity INTEGER NOT NULL,
        limit_price REAL NOT NULL DEFAULT 0, status INTEGER NOT NULL, filled_quantity INTEGER NOT NULL DEFAULT 0,
        average_fill_price REAL NOT NULL DEFAULT 0, rejection_reason TEXT NOT NULL DEFAULT '',
        created_at_ms INTEGER NOT NULL, updated_at_ms INTEGER NOT NULL))");
    q.exec("CREATE INDEX IF NOT EXISTS idx_trading_orders_updated ON trading_orders(updated_at_ms DESC)");
}
}
