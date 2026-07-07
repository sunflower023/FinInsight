#pragma once

#include "storage/Migration.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

namespace fininsight::storage::migrations {

/**
 * @brief V001: 初始建表
 *
 * 创建四张基础表：
 *   - stocks          股票基础信息
 *   - klines          K 线数据（日线）
 *   - watchlist       自选股
 *   - schema_version  迁移版本记录（框架自用）
 */
namespace V001_Initial {

inline void up(QSqlDatabase db) {
    QSqlQuery q(db);

    // ── 股票基础信息 ──────────────────────────────────
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS stocks (
            id         INTEGER PRIMARY KEY AUTOINCREMENT,
            symbol     TEXT    NOT NULL UNIQUE,
            name       TEXT,
            exchange   TEXT,
            currency   TEXT    DEFAULT 'USD',
            last_price REAL    DEFAULT 0,
            updated_at INTEGER DEFAULT 0
        )
    )");

    // ── K 线数据 ──────────────────────────────────────
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS klines (
            id      INTEGER PRIMARY KEY AUTOINCREMENT,
            symbol  TEXT    NOT NULL,
            date    TEXT    NOT NULL,
            open    REAL,
            high    REAL,
            low     REAL,
            close   REAL,
            volume  INTEGER DEFAULT 0,
            UNIQUE(symbol, date)
        )
    )");
    q.exec("CREATE INDEX IF NOT EXISTS idx_klines_symbol ON klines(symbol)");

    // ── 自选股（只存代码，详情通过 JOIN stocks 获取）────
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS watchlist (
            symbol   TEXT PRIMARY KEY,
            added_at INTEGER DEFAULT (strftime('%s','now')),
            note     TEXT DEFAULT ''
        )
    )");

    // ── 迁移版本记录 ──────────────────────────────────
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS schema_version (
            version    INTEGER PRIMARY KEY,
            applied_at INTEGER DEFAULT (strftime('%s','now'))
        )
    )");
    q.exec("INSERT OR REPLACE INTO schema_version (version) VALUES (1)");
}

} // namespace V001_Initial

} // namespace fininsight::storage::migrations
