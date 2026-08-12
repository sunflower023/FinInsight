#include "datahub/Aggregator.h"
#include "datahub/DataHub.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QtGlobal>

#include <algorithm>

namespace fininsight::datahub {
namespace net = fininsight::network;

Aggregator::Aggregator(QObject* parent)
    : QObject(parent)
{}

quint64 Aggregator::createState(const QString& symbol, bool bestOnly, int timeoutMs)
{
    const quint64 id = nextStateId_++;
    AggregateState state;
    state.id = id;
    state.symbol = symbol;
    state.bestOnly = bestOnly;
    state.elapsed.start();
    state.timeoutTimer = new QTimer(this);
    state.timeoutTimer->setSingleShot(true);
    connect(state.timeoutTimer, &QTimer::timeout, this, [this, id]() {
        finishState(id, "Aggregate request timed out");
    });
    states_.insert(id, std::move(state));
    states_[id].timeoutTimer->start(qMax(1, timeoutMs));
    return id;
}

void Aggregator::fetchBest(const QString& symbol,
                           std::function<void(QuoteData)> onDone,
                           int timeoutMs)
{
    const QString normalized = symbol.trimmed().toUpper();
    if (normalized.isEmpty()) {
        emit errorOccurred(symbol, "Symbol is empty");
        return;
    }

    const quint64 id = createState(normalized, true, timeoutMs);
    states_[id].onDone = std::move(onDone);
    startSource(id, "Yahoo", yahooUrl(normalized), &Aggregator::parseYahoo);
    if (isNumericSymbol(normalized)) {
        startSource(id, "EastMoney", eastMoneyUrl(normalized), &Aggregator::parseEastMoney);
        startSource(id, "Sina", sinaUrl(normalized), &Aggregator::parseSina);
    }
}

void Aggregator::fetchAll(const QString& symbol,
                          std::function<void(QuoteData, const QString&)> onEach,
                          int timeoutMs)
{
    const QString normalized = symbol.trimmed().toUpper();
    if (normalized.isEmpty()) {
        emit errorOccurred(symbol, "Symbol is empty");
        return;
    }

    const quint64 id = createState(normalized, false, timeoutMs);
    states_[id].onEach = std::move(onEach);
    startSource(id, "Yahoo", yahooUrl(normalized), &Aggregator::parseYahoo);
    if (isNumericSymbol(normalized)) {
        startSource(id, "EastMoney", eastMoneyUrl(normalized), &Aggregator::parseEastMoney);
        startSource(id, "Sina", sinaUrl(normalized), &Aggregator::parseSina);
    }
}

void Aggregator::startSource(quint64 stateId, const QString& source,
                             const QString& url, Parser parser)
{
    auto stateIt = states_.find(stateId);
    if (stateIt == states_.end() || stateIt->finished) return;
    stateIt->pendingSources.insert(source);

    const auto requestId = net::HttpClient::instance().getAsync(
        url, this,
        [this, stateId, source, parser](const net::HttpResponse& response) {
            handleSourceResponse(stateId, source, parser, response);
        }, 3000);

    if (requestId == net::HttpClient::InvalidRequestId) {
        stateIt->pendingSources.remove(source);
        stateIt->results.append({source, 0, {}, false});
        if (stateIt->pendingSources.isEmpty()) finishState(stateId, "Failed to start any source request");
        return;
    }
    stateIt->requests.insert(requestId);
}

void Aggregator::handleSourceResponse(quint64 stateId, const QString& source,
                                      Parser parser, const net::HttpResponse& response)
{
    auto stateIt = states_.find(stateId);
    if (stateIt == states_.end() || stateIt->finished) return;
    auto& state = stateIt.value();
    state.pendingSources.remove(source);

    QuoteData quote;
    if (response.isSuccess() && !response.body.isEmpty()) {
        quote = parser(response.body, state.symbol);
    }
    const bool valid = validateQuote(quote, state.symbol);
    state.results.append({source, static_cast<int>(state.elapsed.elapsed()), quote, valid});

    if (valid && state.bestOnly) {
        DataHub::instance().publishQuote(quote);
        if (state.onDone) state.onDone(quote);
        emit resultReady(quote, state.results);
        finishState(stateId);
        return;
    }
    if (valid && state.onEach) state.onEach(quote, source);
    if (state.pendingSources.isEmpty()) {
        const bool hasValidResult = std::any_of(
            state.results.cbegin(), state.results.cend(),
            [](const SourceResult& result) { return result.valid; });
        if (hasValidResult) finishState(stateId);
        else finishState(stateId, "All data sources returned invalid data");
    }
}

void Aggregator::finishState(quint64 stateId, const QString& error)
{
    auto it = states_.find(stateId);
    if (it == states_.end() || it->finished) return;
    it->finished = true;
    if (it->timeoutTimer) {
        it->timeoutTimer->stop();
        it->timeoutTimer->deleteLater();
    }
    for (const auto requestId : it->requests) {
        net::HttpClient::instance().cancel(requestId);
    }
    const QString symbol = it->symbol;
    const bool failed = !error.isEmpty();
    states_.erase(it);
    if (failed) emit errorOccurred(symbol, error);
}

bool Aggregator::validateQuote(const QuoteData& quote, const QString& symbol) const
{
    return quote.isValid() && quote.symbol.compare(symbol, Qt::CaseInsensitive) == 0
        && qIsFinite(quote.price) && quote.price > 0.0
        && quote.timestamp > 0;
}

bool Aggregator::isNumericSymbol(const QString& symbol)
{
    static const QRegularExpression re(QStringLiteral("^[0-9]{6}$"));
    return re.match(symbol).hasMatch();
}

QString Aggregator::yahooUrl(const QString& symbol)
{
    return QStringLiteral("https://query1.finance.yahoo.com/v8/finance/chart/%1?interval=2m&range=1d").arg(symbol);
}

QString Aggregator::eastMoneyUrl(const QString& symbol)
{
    const QString secid = symbol.startsWith('6') ? "1." + symbol : "0." + symbol;
    return QStringLiteral("http://push2.eastmoney.com/api/qt/stock/get?secid=%1&fields=f43,f44,f45,f46,f47,f57,f58,f169,f170").arg(secid);
}

QString Aggregator::sinaUrl(const QString& symbol)
{
    const QString code = symbol.startsWith('6') ? "sh" + symbol : "sz" + symbol;
    return QStringLiteral("http://hq.sinajs.cn/list=%1").arg(code);
}

QuoteData Aggregator::parseYahoo(const QByteArray& json, const QString& symbol)
{
    QuoteData q; q.symbol = symbol;
    const auto root = QJsonDocument::fromJson(json).object();
    const auto results = root["chart"].toObject()["result"].toArray();
    if (results.isEmpty()) return q;
    const auto meta = results.first().toObject()["meta"].toObject();
    q.price = meta["regularMarketPrice"].toDouble();
    q.prevClose = meta["previousClose"].toDouble();
    q.open = meta["regularMarketOpen"].toDouble();
    q.high = meta["regularMarketDayHigh"].toDouble();
    q.low = meta["regularMarketDayLow"].toDouble();
    q.currency = meta["currency"].toString();
    q.exchange = meta["exchangeName"].toString();
    q.change = q.price - q.prevClose;
    q.changePercent = q.prevClose > 0 ? q.change / q.prevClose * 100.0 : 0.0;
    q.timestamp = QDateTime::currentMSecsSinceEpoch();
    return q;
}

QuoteData Aggregator::parseEastMoney(const QByteArray& json, const QString& symbol)
{
    QuoteData q; q.symbol = symbol;
    const auto data = QJsonDocument::fromJson(json).object()["data"].toObject();
    if (data.isEmpty()) return q;
    q.price = data["f43"].toDouble() / 100.0;
    q.high = data["f44"].toDouble() / 100.0;
    q.low = data["f45"].toDouble() / 100.0;
    q.open = data["f46"].toDouble() / 100.0;
    q.volume = static_cast<qint64>(data["f47"].toDouble());
    q.name = data["f57"].toString();
    q.change = data["f58"].toDouble() / 100.0;
    q.changePercent = data["f169"].toDouble() / 100.0;
    q.prevClose = data["f170"].toDouble() / 100.0;
    q.currency = "CNY"; q.exchange = symbol.startsWith('6') ? "SSE" : "SZSE";
    q.timestamp = QDateTime::currentMSecsSinceEpoch();
    return q;
}

QuoteData Aggregator::parseSina(const QByteArray& bytes, const QString& symbol)
{
    QuoteData q; q.symbol = symbol;
    const QString response = QString::fromLocal8Bit(bytes);
    const int begin = response.indexOf('\"');
    const int end = response.indexOf('\"', begin + 1);
    if (begin < 0 || end < 0) return q;
    const auto parts = response.mid(begin + 1, end - begin - 1).split(',');
    if (parts.size() < 6) return q;
    q.name = parts.value(0); q.open = parts.value(1).toDouble(); q.prevClose = parts.value(2).toDouble();
    q.price = parts.value(3).toDouble(); q.high = parts.value(4).toDouble(); q.low = parts.value(5).toDouble();
    q.change = q.price - q.prevClose; q.changePercent = q.prevClose > 0 ? q.change / q.prevClose * 100.0 : 0.0;
    q.currency = "CNY"; q.exchange = symbol.startsWith('6') ? "SSE" : "SZSE";
    q.timestamp = QDateTime::currentMSecsSinceEpoch();
    return q;
}

} // namespace fininsight::datahub
