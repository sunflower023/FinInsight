#pragma once

#include <QSqlDatabase>
#include <QSqlQuery>

namespace fininsight::storage::migrations::V003_FindingEvidence {

inline void up(QSqlDatabase db) {
    QSqlQuery q(db);
    q.exec("ALTER TABLE evidence_findings ADD COLUMN evidence_start_ms INTEGER NOT NULL DEFAULT 0");
    q.exec("ALTER TABLE evidence_findings ADD COLUMN evidence_end_ms INTEGER NOT NULL DEFAULT 0");
    q.exec("ALTER TABLE evidence_findings ADD COLUMN evidence_trade_ids TEXT NOT NULL DEFAULT ''");
}

} // namespace fininsight::storage::migrations::V003_FindingEvidence
