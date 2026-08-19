#pragma once

#include "storage/Migration.h"

#include <QSqlDatabase>
#include <QSqlQuery>

namespace fininsight::storage::migrations::V002_Evidence {

inline void up(QSqlDatabase db) {
    QSqlQuery q(db);
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS evidence_snapshots (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            schema_version INTEGER NOT NULL DEFAULT 1,
            source TEXT NOT NULL DEFAULT '',
            price_basis TEXT NOT NULL DEFAULT '',
            start_timestamp_ms INTEGER NOT NULL DEFAULT 0,
            end_timestamp_ms INTEGER NOT NULL DEFAULT 0,
            initial_cash REAL NOT NULL DEFAULT 0,
            cash REAL NOT NULL DEFAULT 0,
            holdings_value REAL NOT NULL DEFAULT 0,
            total_equity REAL NOT NULL DEFAULT 0,
            realized_pnl REAL NOT NULL DEFAULT 0,
            unrealized_pnl REAL NOT NULL DEFAULT 0,
            return_rate REAL NOT NULL DEFAULT 0,
            max_drawdown REAL NOT NULL DEFAULT 0,
            drawdown_start_ms INTEGER NOT NULL DEFAULT 0,
            drawdown_end_ms INTEGER NOT NULL DEFAULT 0,
            created_at INTEGER NOT NULL DEFAULT (strftime('%s','now'))
        )
    )");
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS evidence_trades (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            snapshot_id INTEGER NOT NULL REFERENCES evidence_snapshots(id) ON DELETE CASCADE,
            trade_id INTEGER NOT NULL,
            side INTEGER NOT NULL,
            symbol TEXT NOT NULL,
            quantity INTEGER NOT NULL,
            price REAL NOT NULL,
            fee REAL NOT NULL,
            timestamp_ms INTEGER NOT NULL,
            realized_pnl REAL NOT NULL DEFAULT 0
        )
    )");
    q.exec("CREATE INDEX IF NOT EXISTS idx_evidence_trades_snapshot ON evidence_trades(snapshot_id)");
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS evidence_findings (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            snapshot_id INTEGER NOT NULL REFERENCES evidence_snapshots(id) ON DELETE CASCADE,
            code TEXT NOT NULL,
            severity INTEGER NOT NULL,
            message TEXT NOT NULL,
            measure REAL NOT NULL DEFAULT 0
        )
    )");
    q.exec("CREATE INDEX IF NOT EXISTS idx_evidence_findings_snapshot ON evidence_findings(snapshot_id)");
}

} // namespace fininsight::storage::migrations::V002_Evidence
