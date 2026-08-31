#pragma once

#include "market/Instrument.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace fininsight::market {

class SymbolResolver final {
public:
    struct RegistrationResult {
        bool success = false;
        std::string error;
    };

    static std::string normalize(std::string symbol);
    RegistrationResult registerInstrument(Instrument instrument);
    std::optional<Instrument> resolve(const std::string& symbolOrAlias) const;
    std::optional<std::string> sourceSymbol(const std::string& symbolOrAlias, InstrumentSource source) const;
    std::vector<Instrument> instruments() const;

private:
    std::unordered_map<std::string, Instrument> instruments_;
    std::unordered_map<std::string, std::string> aliases_;
};

} // namespace fininsight::market
