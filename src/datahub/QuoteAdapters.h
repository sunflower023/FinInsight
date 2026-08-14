#pragma once

#include "datahub/QuoteData.h"

#include <QByteArray>
#include <QString>
#include <QStringList>

namespace fininsight::datahub::quote_adapters {

bool isNumericSymbol(const QString& symbol);
QStringList sourcesForSymbol(const QString& symbol);

QString yahooUrl(const QString& symbol);
QString eastMoneyUrl(const QString& symbol);
QString sinaUrl(const QString& symbol);

QuoteData parseYahoo(const QByteArray& json, const QString& symbol);
QuoteData parseEastMoney(const QByteArray& json, const QString& symbol);
QuoteData parseSina(const QByteArray& bytes, const QString& symbol);

} // namespace fininsight::datahub::quote_adapters
