#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace fininsight::market {

enum class QuoteSource {
    Yahoo,
    EastMoney,
    Sina,
};

struct QuoteCandidate {
    std::string symbol;
    double price = 0.0;
    std::int64_t timestampMs = 0;
};

struct SourceFailure {
    QuoteSource source = QuoteSource::Yahoo;
    std::string message;
};

std::string normalizeSymbol(std::string_view symbol);
bool isChinaAShareSymbol(std::string_view symbol);
std::vector<QuoteSource> sourcesForSymbol(std::string_view symbol);
bool isValidQuote(const QuoteCandidate& quote, std::string_view expectedSymbol);
std::string sourceName(QuoteSource source);
std::string summarizeFailures(const std::vector<SourceFailure>& failures);

} // namespace fininsight::market
