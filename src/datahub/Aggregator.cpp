#include "datahub/Aggregator.h"
#include "datahub/DataHub.h"
#include "datahub/QuoteAdapters.h"
#include "market/QuoteRules.h"

#include <QtGlobal>
#include <QStringList>

#include <algorithm>

namespace fininsight::datahub {
namespace net = fininsight::network;
namespace adapters = fininsight::datahub::quote_adapters;

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
        finishState(id, QStringLiteral("Aggregate request timed out"));
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
        emit errorOccurred(symbol, QStringLiteral("Symbol is empty"));
        return;
    }

    const quint64 id = createState(normalized, true, timeoutMs);
    states_[id].onDone = std::move(onDone);
    for (const auto& source : adapters::sourcesForSymbol(normalized)) {
        if (source == QLatin1String("Yahoo")) {
            startSource(id, source, adapters::yahooUrl(normalized), adapters::parseYahoo);
        } else if (source == QLatin1String("EastMoney")) {
            startSource(id, source, adapters::eastMoneyUrl(normalized), adapters::parseEastMoney);
        } else if (source == QLatin1String("Sina")) {
            startSource(id, source, adapters::sinaUrl(normalized), adapters::parseSina);
        }
    }
    if (states_.contains(id) && states_[id].pendingSources.isEmpty()) {
        finishState(id, QStringLiteral("No data source is available for symbol"));
    }
}

void Aggregator::fetchAll(const QString& symbol,
                          std::function<void(QuoteData, const QString&)> onEach,
                          int timeoutMs)
{
    const QString normalized = symbol.trimmed().toUpper();
    if (normalized.isEmpty()) {
        emit errorOccurred(symbol, QStringLiteral("Symbol is empty"));
        return;
    }

    const quint64 id = createState(normalized, false, timeoutMs);
    states_[id].onEach = std::move(onEach);
    for (const auto& source : adapters::sourcesForSymbol(normalized)) {
        if (source == QLatin1String("Yahoo")) {
            startSource(id, source, adapters::yahooUrl(normalized), adapters::parseYahoo);
        } else if (source == QLatin1String("EastMoney")) {
            startSource(id, source, adapters::eastMoneyUrl(normalized), adapters::parseEastMoney);
        } else if (source == QLatin1String("Sina")) {
            startSource(id, source, adapters::sinaUrl(normalized), adapters::parseSina);
        }
    }
    if (states_.contains(id) && states_[id].pendingSources.isEmpty()) {
        finishState(id, QStringLiteral("No data source is available for symbol"));
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
        stateIt->results.append({source, QStringLiteral("Failed to start request"), 0, {}, false});
        if (stateIt->pendingSources.isEmpty()) {
            finishState(stateId, QStringLiteral("Failed to start any source request"));
        }
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
    const QString sourceError = response.isSuccess()
        ? (valid ? QString{} : QStringLiteral("Invalid quote payload"))
        : response.error;
    state.results.append({source, sourceError, static_cast<int>(state.elapsed.elapsed()), quote, valid});

    if (valid && state.bestOnly) {
        const QuoteData best = quote;
        const auto allResults = state.results;
        auto onDone = std::move(state.onDone);
        finishState(stateId);
        DataHub::instance().publishQuote(best);
        if (onDone) onDone(best);
        emit resultReady(best, allResults);
        return;
    }

    auto onEach = state.onEach;
    const QuoteData eachResult = quote;
    if (state.pendingSources.isEmpty()) {
        const bool hasValidResult = std::any_of(
            state.results.cbegin(), state.results.cend(),
            [](const SourceResult& result) { return result.valid; });
        if (hasValidResult) {
            finishState(stateId);
        } else {
            QStringList errors;
            for (const auto& result : state.results) {
                errors.append(result.source + QStringLiteral(": ") + result.error);
            }
            finishState(stateId, QStringLiteral("All data sources failed: ") + errors.join("; "));
        }
    }
    if (valid && onEach) {
        onEach(eachResult, source);
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
    return market::isValidQuote(
        {quote.symbol.toStdString(), quote.price, quote.timestamp},
        symbol.toStdString());
}

} // namespace fininsight::datahub
