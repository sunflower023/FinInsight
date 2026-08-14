#include "datahub/HistoricalPriceAdapter.h"

#include <optional>
#include <vector>

namespace fininsight::datahub {

simulation::HistoricalPriceSeriesResult toHistoricalPriceSeries(
    const QString& expectedSymbol,
    const QVector<KLineData>& bars,
    simulation::HistoricalPriceField priceField)
{
    std::vector<simulation::HistoricalBar> historicalBars;
    historicalBars.reserve(static_cast<std::size_t>(bars.size()));
    for (const KLineData& bar : bars) {
        historicalBars.push_back({
            bar.symbol.toStdString(),
            bar.date.toStdString(),
            bar.close,
            bar.hasAdjustedClose ? std::optional<double>{bar.adjustedClose} : std::nullopt,
        });
    }
    return simulation::buildHistoricalPriceSeries(
        expectedSymbol.toStdString(), historicalBars, priceField);
}

} // namespace fininsight::datahub
