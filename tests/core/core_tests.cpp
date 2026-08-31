#include "market/QuoteRules.h"
#include "simulation/HistoricalPriceSeries.h"
#include "simulation/InvestmentExperiment.h"
#include "simulation/Ledger.h"
#include "analysis/BehaviorAnalyzer.h"
#include "trading/RiskEngine.h"
#include "trading/OrderService.h"
#include "trading/PaperExecutionGateway.h"
#include "market/SymbolResolver.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace {

int failures = 0;

void expect(bool condition, const std::string& message)
{
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

void expectNear(double actual, double expected, const std::string& message)
{
    expect(std::abs(actual - expected) < 1e-8,
           message + " (actual=" + std::to_string(actual)
           + ", expected=" + std::to_string(expected) + ")");
}

void testQuoteRules()
{
    using namespace fininsight::market;
    expect(normalizeSymbol("  aapl \t") == "AAPL", "symbol normalization");
    expect(isChinaAShareSymbol("600519"), "six-digit A-share detection");
    expect(!isChinaAShareSymbol("AAPL"), "US symbol is not A-share");

    const auto chinaSources = sourcesForSymbol("600519");
    expect(chinaSources.size() == 2, "A-share has two quote sources");
    expect(chinaSources[0] == QuoteSource::EastMoney, "EastMoney is first A-share source");
    expect(chinaSources[1] == QuoteSource::Sina, "Sina is second A-share source");

    const auto yahooSources = sourcesForSymbol("aapl");
    expect(yahooSources.size() == 1 && yahooSources[0] == QuoteSource::Yahoo,
           "non-A-share routes to Yahoo");

    expect(isValidQuote({"aapl", 100.0, 1}, "AAPL"), "valid quote accepted");
    expect(!isValidQuote({"MSFT", 100.0, 1}, "AAPL"), "mismatched symbol rejected");
    expect(!isValidQuote({"AAPL", 0.0, 1}, "AAPL"), "zero price rejected");
    expect(!isValidQuote({"AAPL", std::numeric_limits<double>::infinity(), 1}, "AAPL"),
           "non-finite price rejected");
    expect(!isValidQuote({"AAPL", 100.0, 0}, "AAPL"), "missing timestamp rejected");

    const std::string summary = summarizeFailures({
        {QuoteSource::Yahoo, "HTTP 503"},
        {QuoteSource::Sina, "Invalid payload"},
    });
    expect(summary == "Yahoo: HTTP 503; Sina: Invalid payload", "failure summary");
}

void testLedgerBuySellAndValuation()
{
    using namespace fininsight::simulation;
    Ledger ledger(10000.0);

    const auto buy = ledger.execute({TradeSide::Buy, " aapl ", 10, 100.0, 5.0, 1});
    expect(buy.ok(), "buy succeeds");
    expectNear(ledger.cash(), 8995.0, "buy reduces cash including fee");
    expect(ledger.positions().at("AAPL").quantity == 10, "buy creates position");
    expectNear(ledger.positions().at("AAPL").averageCost(), 100.5,
               "buy fee included in average cost");

    const auto secondBuy = ledger.execute({TradeSide::Buy, "AAPL", 10, 120.0, 5.0, 2});
    expect(secondBuy.ok(), "second buy succeeds");
    expectNear(ledger.positions().at("AAPL").averageCost(), 110.5,
               "weighted average cost");

    const auto sell = ledger.execute({TradeSide::Sell, "AAPL", 5, 130.0, 5.0, 3});
    expect(sell.ok(), "partial sell succeeds");
    expectNear(sell.trade.realizedPnl, 92.5, "partial sale realized PnL");
    expectNear(ledger.realizedPnl(), 92.5, "ledger realized PnL");
    expect(ledger.positions().at("AAPL").quantity == 15, "partial sale quantity");
    expectNear(ledger.positions().at("AAPL").totalCost, 1657.5,
               "partial sale removes average cost");

    const auto valuation = ledger.value({{"AAPL", 125.0}});
    expect(valuation.valid, "valuation succeeds with complete prices");
    expectNear(valuation.snapshot.holdingsValue, 1875.0, "holdings market value");
    expectNear(valuation.snapshot.unrealizedPnl, 217.5, "unrealized PnL");
    expectNear(valuation.snapshot.totalPnl, 310.0, "total PnL");
    expectNear(valuation.snapshot.totalEquity, 10310.0, "total equity");
    expectNear(valuation.snapshot.returnRate, 0.031, "return rate");

    const auto finalSell = ledger.execute({TradeSide::Sell, "AAPL", 15, 125.0, 0.0, 4});
    expect(finalSell.ok(), "full sale succeeds");
    expect(ledger.positions().empty(), "full sale removes position");
    expect(ledger.trades().size() == 4, "trade history records all fills");
    const auto emptyValuation = ledger.value({});
    expect(emptyValuation.valid, "empty portfolio valuation succeeds");
    expectNear(emptyValuation.snapshot.totalEquity, ledger.cash(),
               "empty portfolio equity equals cash");
}

void testLedgerFailures()
{
    using namespace fininsight::simulation;
    Ledger ledger(1000.0);
    expect(ledger.execute({TradeSide::Buy, "AAPL", 0, 10.0, 0.0, 1}).error
               == TradeError::InvalidQuantity, "zero quantity rejected");
    expect(ledger.execute({TradeSide::Buy, "AAPL", 101, 10.0, 0.0, 1}).error
               == TradeError::InsufficientCash, "overspending rejected");
    expect(ledger.execute({TradeSide::Sell, "AAPL", 1, 10.0, 0.0, 1}).error
               == TradeError::InsufficientPosition, "short selling rejected");

    expect(ledger.execute({TradeSide::Buy, "MSFT", 10, 10.0, 0.0, 1}).ok(),
           "setup position succeeds");
    expect(!ledger.value({}).valid, "missing valuation price rejected");
    expect(!ledger.value({{"MSFT", -1.0}}).valid, "invalid valuation price rejected");
    expect(ledger.execute({TradeSide::Sell, "MSFT", 1, 1.0, 2.0, 2}).error
               == TradeError::FeeExceedsProceeds,
           "fee larger than sale proceeds rejected");
}

void testInvalidInitialCash()
{
    bool threw = false;
    try {
        fininsight::simulation::Ledger ledger(-1.0);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    expect(threw, "negative initial cash rejected");
}

void testBuyAndHoldExperiment()
{
    using namespace fininsight::simulation;
    const std::vector<PricePoint> prices{
        {100, 8.0},
        {200, 10.0},
        {300, 12.0},
        {400, 9.0},
        {500, 20.0},
    };

    const auto result = runBuyAndHoldExperiment(
        {" aapl ", 1000.0, 150, 450, 5.0}, prices);
    expect(result.ok(), "buy-and-hold experiment succeeds");
    expect(result.symbol == "AAPL", "experiment normalizes symbol");
    expect(result.executedTimestampMs == 200,
           "experiment buys at first price on or after start");
    expectNear(result.executedPrice, 10.0, "experiment execution price");
    expect(result.endingTimestampMs == 400,
           "experiment values at last price on or before end");
    expectNear(result.endingPrice, 9.0, "experiment ending price");
    expect(result.quantity == 99, "experiment buys maximum integer shares after fee");
    expectNear(result.endingCash, 5.0, "experiment retains unspent cash");
    expectNear(result.endingMarketValue, 891.0, "experiment ending market value");
    expectNear(result.endingEquity, 896.0, "experiment ending equity");
    expectNear(result.totalPnl, -104.0, "experiment total PnL");
    expectNear(result.returnRate, -0.104, "experiment return rate");
    expectNear(result.maxDrawdown, 297.0 / 1193.0,
               "experiment max drawdown follows total-equity curve");

    const auto rising = runBuyAndHoldExperiment(
        {"AAPL", 1000.0, 200, 300, 5.0}, prices);
    expect(rising.ok(), "rising experiment succeeds");
    expectNear(rising.endingEquity, 1193.0, "rising experiment ending equity");
    expectNear(rising.returnRate, 0.193, "rising experiment return rate");
    expectNear(rising.maxDrawdown, 0.0, "rising experiment has no drawdown");
}

void testBuyAndHoldExperimentFailures()
{
    using namespace fininsight::simulation;
    const InvestmentExperimentRequest request{"AAPL", 1000.0, 100, 300, 0.0};

    expect(runBuyAndHoldExperiment(request, {}).error
               == InvestmentExperimentError::EmptyPriceSeries,
           "empty experiment price series rejected");
    expect(runBuyAndHoldExperiment(request, {{100, 10.0}, {90, 11.0}}).error
               == InvestmentExperimentError::PriceSeriesNotStrictlyIncreasing,
           "unsorted experiment prices rejected");
    expect(runBuyAndHoldExperiment(request, {{100, 10.0}, {100, 11.0}}).error
               == InvestmentExperimentError::PriceSeriesNotStrictlyIncreasing,
           "duplicate experiment timestamps rejected");
    expect(runBuyAndHoldExperiment(request, {{100, 10.0}, {200, 0.0}}).error
               == InvestmentExperimentError::InvalidPricePoint,
           "zero experiment price rejected");
    expect(runBuyAndHoldExperiment(request,
               {{100, 10.0}, {200, std::numeric_limits<double>::quiet_NaN()}}).error
               == InvestmentExperimentError::InvalidPricePoint,
           "non-finite experiment price rejected");

    const auto noPrice = runBuyAndHoldExperiment(
        {"AAPL", 1000.0, 200, 400, 0.0}, {{100, 10.0}, {500, 11.0}});
    expect(noPrice.error == InvestmentExperimentError::NoPriceInRange,
           "experiment range without prices rejected");
    expect(!noPrice.message.empty(), "experiment failure includes message");

    expect(runBuyAndHoldExperiment(
               {"AAPL", 10.0, 100, 100, 1.0}, {{100, 10.0}}).error
               == InvestmentExperimentError::InsufficientCash,
           "experiment requiring less than one share rejected");
    expect(runBuyAndHoldExperiment(
               {"AAPL", std::numeric_limits<double>::max(), 100, 100, 0.0},
               {{100, 1.0}}).error == InvestmentExperimentError::QuantityOverflow,
           "experiment share quantity overflow rejected");
    expect(runBuyAndHoldExperiment(
               {"", 1000.0, 100, 300, 0.0}, {{100, 10.0}}).error
               == InvestmentExperimentError::InvalidSymbol,
           "empty experiment symbol rejected");
    expect(runBuyAndHoldExperiment(
               {"AAPL", 1000.0, 300, 100, 0.0}, {{100, 10.0}}).error
               == InvestmentExperimentError::InvalidTimeRange,
           "reversed experiment time range rejected");
    expect(runBuyAndHoldExperiment(
               {"AAPL", 1000.0, 100, 300, -1.0}, {{100, 10.0}}).error
               == InvestmentExperimentError::InvalidFee,
           "negative experiment fee rejected");
}

void testHistoricalPriceSeries()
{
    using namespace fininsight::simulation;

    expect(tradingDateToTimestampMs("2024-01-01") == 1704067200000LL,
           "ISO trading date converts to UTC midnight");
    expect(tradingDateToTimestampMs("2024-02-29").has_value(),
           "leap-day trading date accepted");
    expect(!tradingDateToTimestampMs("2023-02-29").has_value(),
           "invalid leap-day trading date rejected");
    expect(!tradingDateToTimestampMs("2024-1-01").has_value(),
           "non-canonical trading date rejected");

    const std::vector<HistoricalBar> bars{
        {"aapl", "2024-01-02", 100.0, 98.0},
        {" AAPL ", "2024-01-03", 110.0, 108.0},
        {"AAPL", "2024-01-05", 90.0, 89.0},
    };
    const auto closeSeries = buildHistoricalPriceSeries(" aapl ", bars);
    expect(closeSeries.ok(), "close-price historical series succeeds");
    expect(closeSeries.symbol == "AAPL", "historical series normalizes symbol");
    expect(closeSeries.priceField == HistoricalPriceField::Close,
           "historical series records close convention");
    expect(closeSeries.prices.size() == 3, "historical series preserves all bars");
    expectNear(closeSeries.prices[0].price, 100.0, "historical close selected");
    expect(closeSeries.prices[1].timestampMs > closeSeries.prices[0].timestampMs,
           "historical timestamps are increasing");

    const auto adjustedSeries = buildHistoricalPriceSeries(
        "AAPL", bars, HistoricalPriceField::AdjustedClose);
    expect(adjustedSeries.ok(), "adjusted historical series succeeds");
    expectNear(adjustedSeries.prices[0].price, 98.0,
               "adjusted historical price selected");

    const auto experiment = runBuyAndHoldExperiment(
        {"AAPL", 1000.0, closeSeries.prices.front().timestampMs,
         closeSeries.prices.back().timestampMs, 0.0},
        closeSeries.prices);
    expect(experiment.ok(), "converted historical series runs experiment");
    expect(experiment.quantity == 10, "converted series experiment quantity");
    expectNear(experiment.endingEquity, 900.0,
               "converted series experiment ending equity");
    expectNear(experiment.maxDrawdown, 200.0 / 1100.0,
               "converted series experiment drawdown");
}

void testHistoricalPriceSeriesFailures()
{
    using namespace fininsight::simulation;
    const std::vector<HistoricalBar> valid{{"AAPL", "2024-01-02", 100.0, 99.0}};

    expect(buildHistoricalPriceSeries("", valid).error
               == HistoricalPriceSeriesError::InvalidExpectedSymbol,
           "empty expected historical symbol rejected");
    expect(buildHistoricalPriceSeries("AAPL", {}).error
               == HistoricalPriceSeriesError::EmptyBars,
           "empty historical bars rejected");
    expect(buildHistoricalPriceSeries("AAPL",
               {{"MSFT", "2024-01-02", 100.0, 99.0}}).error
               == HistoricalPriceSeriesError::SymbolMismatch,
           "mismatched historical symbol rejected");
    expect(buildHistoricalPriceSeries("AAPL",
               {{"AAPL", "bad-date", 100.0, 99.0}}).error
               == HistoricalPriceSeriesError::InvalidTradingDate,
           "invalid historical date rejected");
    expect(buildHistoricalPriceSeries("AAPL", {
               {"AAPL", "2024-01-03", 100.0, 99.0},
               {"AAPL", "2024-01-02", 101.0, 100.0},
           }).error == HistoricalPriceSeriesError::TradingDatesNotStrictlyIncreasing,
           "unsorted historical dates rejected");
    expect(buildHistoricalPriceSeries("AAPL", {
               {"AAPL", "2024-01-02", 100.0, 99.0},
               {"AAPL", "2024-01-02", 101.0, 100.0},
           }).error == HistoricalPriceSeriesError::TradingDatesNotStrictlyIncreasing,
           "duplicate historical dates rejected");
    expect(buildHistoricalPriceSeries("AAPL",
               {{"AAPL", "2024-01-02", 0.0, 99.0}}).error
               == HistoricalPriceSeriesError::InvalidClose,
           "invalid historical close rejected");
    expect(buildHistoricalPriceSeries("AAPL",
               {{"AAPL", "2024-01-02", 100.0, std::nullopt}},
               HistoricalPriceField::AdjustedClose).error
               == HistoricalPriceSeriesError::MissingAdjustedClose,
           "missing adjusted historical close rejected");
    expect(buildHistoricalPriceSeries("AAPL",
               {{"AAPL", "2024-01-02", 100.0,
                 std::numeric_limits<double>::infinity()}},
               HistoricalPriceField::AdjustedClose).error
               == HistoricalPriceSeriesError::InvalidAdjustedClose,
           "invalid adjusted historical close rejected");
}

void testBehaviorAnalyzer()
{
    using namespace fininsight::analysis;
    using namespace fininsight::simulation;
    EvidenceSnapshot evidence;
    evidence.startTimestampMs = 0;
    evidence.endTimestampMs = 5 * 86400000;
    evidence.maxDrawdown = 0.15;
    evidence.drawdownStartTimestampMs = 2 * 86400000;
    evidence.drawdownEndTimestampMs = 4 * 86400000;
    evidence.trades = {
        {1, TradeSide::Buy, "AAPL", 10, 100.0, 0.0, 2 * 86400000, 0.0},
        {2, TradeSide::Buy, "AAPL", 10, 100.0, 0.0, 3 * 86400000, 0.0},
        {3, TradeSide::Sell, "AAPL", 10, 101.0, 0.0, 4 * 86400000, 10.0},
        {4, TradeSide::Sell, "AAPL", 10, 90.0, 0.0, 4 * 86400000, -100.0},
    };
    const auto report = analyzeBehavior(evidence);
    expect(report.tradeCount == 4, "behavior trade count");
    expect(report.symbolCount == 1, "behavior symbol count");
    expectNear(report.maxSymbolConcentration, 1.0, "behavior concentration");
    expect(report.tradesDuringDrawdown == 4, "behavior drawdown trades");
    expect(report.findings.size() == 3, "behavior findings are deterministic");
}

void testTradingStateAndRisk()
{
    using namespace fininsight::trading;
    Order order;
    order.request.quantity = 10;
    order.status = OrderStatus::Accepted;
    expect(statusAfterFill(order, 4) == OrderStatus::PartiallyFilled, "partial fill status");
    order.filledQuantity = 4;
    expect(statusAfterFill(order, 6) == OrderStatus::Filled, "complete fill status");
    expect(isTerminal(OrderStatus::Filled), "filled is terminal");
    expect(!isTerminal(OrderStatus::Unknown), "unknown requires reconciliation");

    RiskLimits limits;
    limits.maxOrderNotional = 1000.0;
    limits.maxOrderQuantity = 20;
    limits.maxSymbolExposure = 1500.0;
    limits.maxDailyOrders = 5;
    limits.maxDailyLoss = 200.0;
    limits.quoteFreshnessMs = 1000;
    RiskEngine engine(limits);
    OrderRequest request;
    request.clientOrderId = "order-1";
    request.symbol = "AAPL";
    request.quantity = 5;
    RiskContext context;
    expect(engine.check(request, 100.0, context).approved, "valid order approved");

    context.killSwitch = true;
    expect(!engine.check(request, 100.0, context).approved, "kill switch rejects");
    context.killSwitch = false;
    context.marketOpen = false;
    expect(!engine.check(request, 100.0, context).approved, "closed market rejects");
    context.marketOpen = true;
    context.quoteAgeMs = 1001;
    expect(!engine.check(request, 100.0, context).approved, "stale quote rejects");
    context.quoteAgeMs = 0;
    context.dailyOrderCount = 5;
    expect(!engine.check(request, 100.0, context).approved, "daily count rejects");
    context.dailyOrderCount = 0;
    context.dailyRealizedLoss = 200.0;
    expect(!engine.check(request, 100.0, context).approved, "daily loss rejects");
    context.dailyRealizedLoss = 0.0;
    context.currentSymbolExposure = 1100.0;
    expect(!engine.check(request, 100.0, context).approved, "symbol exposure rejects");
    context.currentSymbolExposure = 0.0;
    request.quantity = 21;
    expect(!engine.check(request, 100.0, context).approved, "quantity rejects");
}

void testPaperExecutionAndOrderService()
{
    using namespace fininsight::trading;
    PaperExecutionGateway gateway(10000.0, 0.001);
    gateway.onQuote("AAPL", 100.0, 1000);
    OrderService service(gateway, RiskEngine{});
    RiskContext context;

    OrderRequest buy;
    buy.clientOrderId = "buy-1"; buy.symbol = "AAPL"; buy.quantity = 10; buy.timestampMs = 1000;
    const auto filled = service.submit(buy, 100.0, context);
    expect(filled.status == OrderStatus::Filled, "paper market buy fills");
    expect(gateway.position("AAPL") == 10, "paper position updated");
    expectNear(gateway.account().cash, 8999.0, "paper fee and cash applied");

    const auto duplicate = service.submit(buy, 100.0, context);
    expect(duplicate.status == OrderStatus::Rejected, "duplicate client order rejected");

    OrderRequest limit;
    limit.clientOrderId = "limit-1"; limit.symbol = "AAPL"; limit.quantity = 2;
    limit.type = OrderType::Limit; limit.limitPrice = 95.0;
    const auto pending = service.submit(limit, 100.0, context);
    expect(pending.status == OrderStatus::Accepted, "paper limit waits for price");
    gateway.onQuote("AAPL", 94.0, 2000);
    expect(gateway.find("limit-1")->status == OrderStatus::Filled, "paper limit fills on tick");

    OrderRequest cancelOrder;
    cancelOrder.clientOrderId = "cancel-1"; cancelOrder.symbol = "AAPL"; cancelOrder.quantity = 1;
    cancelOrder.type = OrderType::Limit; cancelOrder.limitPrice = 80.0;
    expect(service.submit(cancelOrder, 94.0, context).status == OrderStatus::Accepted, "cancel target accepted");
    expect(service.cancel("cancel-1"), "paper order cancelled");
    gateway.onQuote("AAPL", 79.0, 3000);
    expect(gateway.find("cancel-1")->status == OrderStatus::Cancelled, "cancelled order remains cancelled");
}

void testSymbolResolver()
{
    using namespace fininsight::market;
    SymbolResolver resolver;
    Instrument apple;
    apple.canonicalSymbol = " aapl ";
    apple.exchange = "NASDAQ";
    apple.currency = "USD";
    apple.aliases = {"apple", "us:aapl"};
    apple.sourceSymbols = {{InstrumentSource::Yahoo, "aapl"}, {InstrumentSource::Alpaca, "AAPL"}};
    expect(resolver.registerInstrument(apple).success, "instrument registration");
    expect(SymbolResolver::normalize(" msft ") == "MSFT", "resolver normalization");
    expect(resolver.resolve(" Apple " )->canonicalSymbol == "AAPL", "instrument alias resolution");
    expect(resolver.sourceSymbol("us:aapl", InstrumentSource::Yahoo) == std::optional<std::string>("AAPL"), "source symbol mapping");

    Instrument conflict; conflict.canonicalSymbol="MSFT"; conflict.exchange="NASDAQ"; conflict.currency="USD"; conflict.aliases={"APPLE"}; conflict.sourceSymbols={{InstrumentSource::Alpaca,"MSFT"}};
    expect(!resolver.registerInstrument(conflict).success, "alias conflict rejected");
    expect(!resolver.sourceSymbol("AAPL", static_cast<InstrumentSource>(99)).has_value(), "missing source mapping fails");

    apple.aliases = {"apple-inc"};
    expect(resolver.registerInstrument(apple).success, "instrument update");
    expect(!resolver.resolve("apple").has_value(), "stale alias removed on update");
    expect(resolver.resolve("APPLE-INC").has_value(), "updated alias resolves");
    expect(resolver.instruments().size() == 1, "instrument update does not duplicate");
}

} // namespace

int main()
{
    testQuoteRules();
    testLedgerBuySellAndValuation();
    testLedgerFailures();
    testInvalidInitialCash();
    testBuyAndHoldExperiment();
    testBuyAndHoldExperimentFailures();
    testHistoricalPriceSeries();
    testHistoricalPriceSeriesFailures();
    testBehaviorAnalyzer();
    testTradingStateAndRisk();
    testPaperExecutionAndOrderService();
    testSymbolResolver();

    if (failures == 0) {
        std::cout << "All core tests passed\n";
        return 0;
    }
    std::cerr << failures << " core test(s) failed\n";
    return 1;
}
