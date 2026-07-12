#pragma once

#include <QString>
#include <QMetaType>

namespace fininsight::datahub {

/**
 * @brief 实时报价快照 — 所有数据源和面板统一使用此结构体
 */
struct QuoteData {
    QString symbol;           // "AAPL" / "600519"
    QString name;             // "Apple Inc." / "贵州茅台"
    QString exchange;         // "NASDAQ" / "SSE"
    QString currency;         // "USD" / "CNY"
    double  price        = 0.0;
    double  change       = 0.0;   // 涨跌额
    double  changePercent = 0.0;  // 涨跌幅 %
    double  open         = 0.0;
    double  high         = 0.0;
    double  low          = 0.0;
    double  prevClose    = 0.0;
    qint64  volume       = 0;
    qint64  timestamp    = 0;     // UNIX 毫秒时间戳

    bool isValid() const { return !symbol.isEmpty() && price > 0; }
};

/**
 * @brief 单根 K 线数据
 */
struct KLineData {
    QString symbol;
    QString date;          // "2026-07-12"
    double  open   = 0.0;
    double  high   = 0.0;
    double  low    = 0.0;
    double  close  = 0.0;
    qint64  volume = 0;
};

} // namespace fininsight::datahub

// 注册自定义类型，使 QVariant<> 和跨线程信号槽可用
Q_DECLARE_METATYPE(fininsight::datahub::QuoteData)
