#include "datahub/QuoteAdapters.h"
#include "market/QuoteRules.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace fininsight::datahub::quote_adapters {

bool isNumericSymbol(const QString& symbol)
{
    return market::isChinaAShareSymbol(symbol.toStdString());
}

QStringList sourcesForSymbol(const QString& symbol)
{
    QStringList result;
    for (const auto source : market::sourcesForSymbol(symbol.toStdString())) {
        result.append(QString::fromStdString(market::sourceName(source)));
    }
    return result;
}

QString yahooUrl(const QString& symbol)
{
    return QStringLiteral(
        "https://query1.finance.yahoo.com/v8/finance/chart/%1?interval=2m&range=1d")
        .arg(symbol);
}

QString eastMoneyUrl(const QString& symbol)
{
    const QString secid = symbol.startsWith('6') ? "1." + symbol : "0." + symbol;
    return QStringLiteral(
        "http://push2.eastmoney.com/api/qt/stock/get?secid=%1&fields="
        "f43,f44,f45,f46,f47,f57,f58,f169,f170")
        .arg(secid);
}

QString sinaUrl(const QString& symbol)
{
    const QString code = symbol.startsWith('6') ? "sh" + symbol : "sz" + symbol;
    return QStringLiteral("http://hq.sinajs.cn/list=%1").arg(code);
}

QuoteData parseYahoo(const QByteArray& json, const QString& symbol)
{
    QuoteData quote;
    quote.symbol = symbol;
    const auto root = QJsonDocument::fromJson(json).object();
    const auto results = root["chart"].toObject()["result"].toArray();
    if (results.isEmpty()) return quote;

    const auto meta = results.first().toObject()["meta"].toObject();
    quote.price = meta["regularMarketPrice"].toDouble();
    quote.prevClose = meta["previousClose"].toDouble();
    quote.open = meta["regularMarketOpen"].toDouble();
    quote.high = meta["regularMarketDayHigh"].toDouble();
    quote.low = meta["regularMarketDayLow"].toDouble();
    quote.volume = static_cast<qint64>(meta["regularMarketVolume"].toDouble());
    quote.currency = meta["currency"].toString();
    quote.exchange = meta["exchangeName"].toString();
    quote.change = quote.price - quote.prevClose;
    quote.changePercent = quote.prevClose > 0
        ? quote.change / quote.prevClose * 100.0 : 0.0;
    quote.timestamp = QDateTime::currentMSecsSinceEpoch();
    return quote;
}

QuoteData parseEastMoney(const QByteArray& json, const QString& symbol)
{
    QuoteData quote;
    quote.symbol = symbol;
    const auto data = QJsonDocument::fromJson(json).object()["data"].toObject();
    if (data.isEmpty()) return quote;

    quote.price = data["f43"].toDouble() / 100.0;
    quote.high = data["f44"].toDouble() / 100.0;
    quote.low = data["f45"].toDouble() / 100.0;
    quote.open = data["f46"].toDouble() / 100.0;
    quote.volume = static_cast<qint64>(data["f47"].toDouble());
    quote.name = data["f57"].toString();
    quote.change = data["f58"].toDouble() / 100.0;
    quote.changePercent = data["f169"].toDouble() / 100.0;
    quote.prevClose = data["f170"].toDouble() / 100.0;
    quote.currency = "CNY";
    quote.exchange = symbol.startsWith('6') ? "SSE" : "SZSE";
    quote.timestamp = QDateTime::currentMSecsSinceEpoch();
    return quote;
}

QuoteData parseSina(const QByteArray& bytes, const QString& symbol)
{
    QuoteData quote;
    quote.symbol = symbol;
    const QString response = QString::fromLocal8Bit(bytes);
    const int begin = response.indexOf('"');
    const int end = response.indexOf('"', begin + 1);
    if (begin < 0 || end < 0) return quote;

    const auto parts = response.mid(begin + 1, end - begin - 1).split(',');
    if (parts.size() < 6) return quote;
    quote.name = parts.value(0);
    quote.open = parts.value(1).toDouble();
    quote.prevClose = parts.value(2).toDouble();
    quote.price = parts.value(3).toDouble();
    quote.high = parts.value(4).toDouble();
    quote.low = parts.value(5).toDouble();
    quote.change = quote.price - quote.prevClose;
    quote.changePercent = quote.prevClose > 0
        ? quote.change / quote.prevClose * 100.0 : 0.0;
    quote.currency = "CNY";
    quote.exchange = symbol.startsWith('6') ? "SSE" : "SZSE";
    quote.timestamp = QDateTime::currentMSecsSinceEpoch();
    return quote;
}

} // namespace fininsight::datahub::quote_adapters
