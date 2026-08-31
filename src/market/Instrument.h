#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace fininsight::market {

enum class AssetType { Stock };
enum class InstrumentSource { Yahoo, Alpaca };

struct Instrument {
    std::string canonicalSymbol;
    std::string exchange;
    std::string currency;
    AssetType assetType = AssetType::Stock;
    std::vector<std::string> aliases;
    std::unordered_map<InstrumentSource, std::string> sourceSymbols;
};

} // namespace fininsight::market
