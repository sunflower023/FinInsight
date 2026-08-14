#include "market/QuoteRules.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>

namespace fininsight::market {

std::string normalizeSymbol(std::string_view symbol)
{
    const auto first = std::find_if_not(symbol.begin(), symbol.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    const auto last = std::find_if_not(symbol.rbegin(), symbol.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();
    if (first >= last) return {};

    std::string normalized(first, last);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return normalized;
}

bool isChinaAShareSymbol(std::string_view symbol)
{
    const std::string normalized = normalizeSymbol(symbol);
    return normalized.size() == 6
        && std::all_of(normalized.begin(), normalized.end(), [](unsigned char ch) {
            return std::isdigit(ch) != 0;
        });
}

std::vector<QuoteSource> sourcesForSymbol(std::string_view symbol)
{
    if (isChinaAShareSymbol(symbol)) {
        return {QuoteSource::EastMoney, QuoteSource::Sina};
    }
    return {QuoteSource::Yahoo};
}

bool isValidQuote(const QuoteCandidate& quote, std::string_view expectedSymbol)
{
    return !quote.symbol.empty()
        && normalizeSymbol(quote.symbol) == normalizeSymbol(expectedSymbol)
        && std::isfinite(quote.price)
        && quote.price > 0.0
        && quote.timestampMs > 0;
}

std::string sourceName(QuoteSource source)
{
    switch (source) {
    case QuoteSource::Yahoo: return "Yahoo";
    case QuoteSource::EastMoney: return "EastMoney";
    case QuoteSource::Sina: return "Sina";
    }
    return "Unknown";
}

std::string summarizeFailures(const std::vector<SourceFailure>& failures)
{
    std::ostringstream stream;
    for (std::size_t index = 0; index < failures.size(); ++index) {
        if (index > 0) stream << "; ";
        stream << sourceName(failures[index].source) << ": "
               << (failures[index].message.empty() ? "Unknown error" : failures[index].message);
    }
    return stream.str();
}

} // namespace fininsight::market
