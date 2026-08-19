#include "storage/EvidenceSnapshotRepository.h"

#include "storage/Database.h"

#include <QSqlQuery>

#include <algorithm>

namespace fininsight::storage {

EvidenceSnapshotRepository::EvidenceSnapshotRepository(Database& database)
    : database_(database) {}

std::optional<std::int64_t> EvidenceSnapshotRepository::save(
    const analysis::EvidenceSnapshot& evidence,
    const analysis::BehaviorReport& report) {
    if (!database_.beginTransaction()) return std::nullopt;

    QSqlQuery snapshot(database_.mainConnection());
    snapshot.prepare(R"(
        INSERT INTO evidence_snapshots
        (schema_version, source, price_basis, start_timestamp_ms, end_timestamp_ms, initial_cash, cash, holdings_value,
         total_equity, realized_pnl, unrealized_pnl, return_rate, max_drawdown,
         drawdown_start_ms, drawdown_end_ms)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    const auto& portfolio = evidence.portfolio;
    snapshot.addBindValue(static_cast<int>(evidence.schemaVersion));
    snapshot.addBindValue(QString::fromStdString(evidence.source));
    snapshot.addBindValue(QString::fromStdString(evidence.priceBasis));
    snapshot.addBindValue(qlonglong(evidence.startTimestampMs));
    snapshot.addBindValue(qlonglong(evidence.endTimestampMs));
    snapshot.addBindValue(portfolio.initialCash);
    snapshot.addBindValue(portfolio.cash);
    snapshot.addBindValue(portfolio.holdingsValue);
    snapshot.addBindValue(portfolio.totalEquity);
    snapshot.addBindValue(portfolio.realizedPnl);
    snapshot.addBindValue(portfolio.unrealizedPnl);
    snapshot.addBindValue(portfolio.returnRate);
    snapshot.addBindValue(evidence.maxDrawdown);
    snapshot.addBindValue(qlonglong(evidence.drawdownStartTimestampMs));
    snapshot.addBindValue(qlonglong(evidence.drawdownEndTimestampMs));
    if (!snapshot.exec()) {
        database_.rollback();
        return std::nullopt;
    }
    const auto snapshotId = snapshot.lastInsertId().toLongLong();

    QSqlQuery trade(database_.mainConnection());
    trade.prepare(R"(
        INSERT INTO evidence_trades
        (snapshot_id, trade_id, side, symbol, quantity, price, fee, timestamp_ms, realized_pnl)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    for (const auto& item : evidence.trades) {
        trade.bindValue(0, snapshotId);
        trade.bindValue(1, qulonglong(item.id));
        trade.bindValue(2, item.side == simulation::TradeSide::Buy ? 0 : 1);
        trade.bindValue(3, QString::fromStdString(item.symbol));
        trade.bindValue(4, qlonglong(item.quantity));
        trade.bindValue(5, item.price);
        trade.bindValue(6, item.fee);
        trade.bindValue(7, qlonglong(item.timestampMs));
        trade.bindValue(8, item.realizedPnl);
        if (!trade.exec()) {
            database_.rollback();
            return std::nullopt;
        }
    }

    QSqlQuery finding(database_.mainConnection());
    finding.prepare(R"(
        INSERT INTO evidence_findings
        (snapshot_id, code, severity, message, measure, evidence_start_ms, evidence_end_ms, evidence_trade_ids)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    )");
    for (const auto& item : report.findings) {
        finding.bindValue(0, snapshotId);
        finding.bindValue(1, QString::fromStdString(item.code));
        finding.bindValue(2, item.severity == analysis::FindingSeverity::Warning ? 1 : 0);
        finding.bindValue(3, QString::fromStdString(item.message));
        finding.bindValue(4, item.measure);
        finding.bindValue(5, qlonglong(item.evidenceStartTimestampMs));
        finding.bindValue(6, qlonglong(item.evidenceEndTimestampMs));
        QStringList ids;
        for (const auto id : item.evidenceTradeIds) ids.push_back(QString::number(qulonglong(id)));
        const auto encodedIds = ids.join(',');
        finding.bindValue(7, encodedIds.isNull() ? QStringLiteral("") : encodedIds);
        if (!finding.exec()) {
            database_.rollback();
            return std::nullopt;
        }
    }
    if (!database_.commit()) {
        database_.rollback();
        return std::nullopt;
    }
    return snapshotId;
}

std::int64_t EvidenceSnapshotRepository::count() const {
    QSqlQuery query(database_.mainConnection());
    if (!query.exec("SELECT COUNT(*) FROM evidence_snapshots") || !query.next()) return 0;
    return query.value(0).toLongLong();
}

std::vector<EvidenceSnapshotRepository::Summary> EvidenceSnapshotRepository::recent(int limit) const {
    std::vector<Summary> result;
    limit = std::clamp(limit, 1, 1000);
    QSqlQuery query(database_.mainConnection());
    query.prepare(R"(
        SELECT id, start_timestamp_ms, end_timestamp_ms, total_equity,
               return_rate, max_drawdown, source, price_basis
        FROM evidence_snapshots ORDER BY id DESC LIMIT ?
    )");
    query.addBindValue(limit);
    if (!query.exec()) return result;
    while (query.next()) {
        Summary item;
        item.id = query.value(0).toLongLong();
        item.startTimestampMs = query.value(1).toLongLong();
        item.endTimestampMs = query.value(2).toLongLong();
        item.totalEquity = query.value(3).toDouble();
        item.returnRate = query.value(4).toDouble();
        item.maxDrawdown = query.value(5).toDouble();
        item.source = query.value(6).toString().toStdString();
        item.priceBasis = query.value(7).toString().toStdString();
        result.push_back(std::move(item));
    }
    return result;
}

bool EvidenceSnapshotRepository::remove(std::int64_t snapshotId) {
    QSqlQuery query(database_.mainConnection());
    query.prepare("DELETE FROM evidence_snapshots WHERE id = ?");
    query.addBindValue(qlonglong(snapshotId));
    return query.exec() && query.numRowsAffected() == 1;
}

std::optional<EvidenceSnapshotRepository::Detail> EvidenceSnapshotRepository::load(
    std::int64_t snapshotId) const {
    Detail detail;
    QSqlQuery snapshot(database_.mainConnection());
    snapshot.prepare(R"(
        SELECT id, start_timestamp_ms, end_timestamp_ms, total_equity,
               return_rate, max_drawdown, source, price_basis
        FROM evidence_snapshots WHERE id = ?
    )");
    snapshot.addBindValue(qlonglong(snapshotId));
    if (!snapshot.exec() || !snapshot.next()) return std::nullopt;
    detail.summary.id = snapshot.value(0).toLongLong();
    detail.summary.startTimestampMs = snapshot.value(1).toLongLong();
    detail.summary.endTimestampMs = snapshot.value(2).toLongLong();
    detail.summary.totalEquity = snapshot.value(3).toDouble();
    detail.summary.returnRate = snapshot.value(4).toDouble();
    detail.summary.maxDrawdown = snapshot.value(5).toDouble();
    detail.summary.source = snapshot.value(6).toString().toStdString();
    detail.summary.priceBasis = snapshot.value(7).toString().toStdString();

    QSqlQuery trades(database_.mainConnection());
    trades.prepare(R"(
        SELECT trade_id, side, symbol, quantity, price, fee, timestamp_ms, realized_pnl
        FROM evidence_trades WHERE snapshot_id = ? ORDER BY timestamp_ms, id
    )");
    trades.addBindValue(qlonglong(snapshotId));
    if (!trades.exec()) return std::nullopt;
    while (trades.next()) {
        simulation::Trade trade;
        trade.id = trades.value(0).toULongLong();
        trade.side = trades.value(1).toInt() == 0 ? simulation::TradeSide::Buy : simulation::TradeSide::Sell;
        trade.symbol = trades.value(2).toString().toStdString();
        trade.quantity = trades.value(3).toLongLong();
        trade.price = trades.value(4).toDouble();
        trade.fee = trades.value(5).toDouble();
        trade.timestampMs = trades.value(6).toLongLong();
        trade.realizedPnl = trades.value(7).toDouble();
        detail.trades.push_back(std::move(trade));
    }

    QSqlQuery findings(database_.mainConnection());
    findings.prepare("SELECT code, severity, message, measure, evidence_start_ms, evidence_end_ms, evidence_trade_ids FROM evidence_findings WHERE snapshot_id = ? ORDER BY id");
    findings.addBindValue(qlonglong(snapshotId));
    if (!findings.exec()) return std::nullopt;
    while (findings.next()) {
        analysis::BehaviorFinding finding;
        finding.code = findings.value(0).toString().toStdString();
        finding.severity = findings.value(1).toInt() == 1
            ? analysis::FindingSeverity::Warning : analysis::FindingSeverity::Info;
        finding.message = findings.value(2).toString().toStdString();
        finding.measure = findings.value(3).toDouble();
        finding.evidenceStartTimestampMs = findings.value(4).toLongLong();
        finding.evidenceEndTimestampMs = findings.value(5).toLongLong();
        const auto ids = findings.value(6).toString().split(',', Qt::SkipEmptyParts);
        for (const auto& id : ids) finding.evidenceTradeIds.push_back(id.toULongLong());
        detail.findings.push_back(std::move(finding));
    }
    return detail;
}

} // namespace fininsight::storage
