#pragma once
#include "trading/TradingTypes.h"
#include <vector>
namespace fininsight::storage { class Database;
class TradingOrderRepository final {
public:
    explicit TradingOrderRepository(Database& database);
    bool save(const trading::Order& order, const char* environment = "paper");
    std::vector<trading::Order> recent(int limit = 100) const;
private: Database& database_;
};
}
