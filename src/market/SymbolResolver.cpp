#include "market/SymbolResolver.h"

#include <algorithm>
#include <cctype>
#include <unordered_set>

namespace fininsight::market {

std::string SymbolResolver::normalize(std::string symbol)
{
    const auto first = symbol.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = symbol.find_last_not_of(" \t\r\n");
    symbol = symbol.substr(first, last - first + 1);
    std::transform(symbol.begin(), symbol.end(), symbol.begin(), [](unsigned char character) {
        return static_cast<char>(std::toupper(character));
    });
    return symbol;
}

SymbolResolver::RegistrationResult SymbolResolver::registerInstrument(Instrument instrument)
{
    instrument.canonicalSymbol = normalize(std::move(instrument.canonicalSymbol));
    if (instrument.canonicalSymbol.empty()) return {false, "Canonical symbol is required"};
    if (instrument.exchange.empty()) return {false, "Exchange is required"};
    if (instrument.currency.empty()) return {false, "Currency is required"};

    std::unordered_set<std::string> keys{instrument.canonicalSymbol};
    for (auto& alias : instrument.aliases) {
        alias = normalize(std::move(alias));
        if (!alias.empty()) keys.insert(alias);
    }
    for (auto& [source, sourceSymbol] : instrument.sourceSymbols) {
        sourceSymbol = normalize(std::move(sourceSymbol));
        if (sourceSymbol.empty()) return {false, "Source symbol cannot be empty"};
    }

    for (const auto& key : keys) {
        const auto existing = aliases_.find(key);
        if (existing != aliases_.end() && existing->second != instrument.canonicalSymbol)
            return {false, "Symbol or alias conflicts with " + existing->second};
    }

    const auto old = instruments_.find(instrument.canonicalSymbol);
    if (old != instruments_.end()) {
        for (auto it = aliases_.begin(); it != aliases_.end();) {
            if (it->second == instrument.canonicalSymbol) it = aliases_.erase(it);
            else ++it;
        }
    }
    for (const auto& key : keys) aliases_[key] = instrument.canonicalSymbol;
    instruments_[instrument.canonicalSymbol] = std::move(instrument);
    return {true, {}};
}

std::optional<Instrument> SymbolResolver::resolve(const std::string& symbolOrAlias) const
{
    const auto alias = aliases_.find(normalize(symbolOrAlias));
    if (alias == aliases_.end()) return std::nullopt;
    const auto instrument = instruments_.find(alias->second);
    return instrument == instruments_.end() ? std::nullopt : std::optional<Instrument>(instrument->second);
}

std::optional<std::string> SymbolResolver::sourceSymbol(const std::string& symbolOrAlias, InstrumentSource source) const
{
    const auto instrument = resolve(symbolOrAlias);
    if (!instrument) return std::nullopt;
    const auto mapped = instrument->sourceSymbols.find(source);
    return mapped == instrument->sourceSymbols.end() ? std::nullopt : std::optional<std::string>(mapped->second);
}

std::vector<Instrument> SymbolResolver::instruments() const
{
    std::vector<Instrument> result;
    result.reserve(instruments_.size());
    for (const auto& [symbol, instrument] : instruments_) result.push_back(instrument);
    std::sort(result.begin(), result.end(), [](const Instrument& lhs, const Instrument& rhs) { return lhs.canonicalSymbol < rhs.canonicalSymbol; });
    return result;
}

} // namespace fininsight::market
