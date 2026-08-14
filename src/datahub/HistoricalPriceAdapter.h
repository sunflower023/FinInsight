#pragma once

#include "datahub/QuoteData.h"
#include "simulation/HistoricalPriceSeries.h"

#include <QVector>

namespace fininsight::datahub {

simulation::HistoricalPriceSeriesResult toHistoricalPriceSeries(
    const QString& expectedSymbol,
    const QVector<KLineData>& bars,
    simulation::HistoricalPriceField priceField = simulation::HistoricalPriceField::Close);

} // namespace fininsight::datahub
