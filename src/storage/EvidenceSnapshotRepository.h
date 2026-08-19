#pragma once

#include "analysis/BehaviorAnalyzer.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace fininsight::storage {

class Database;

class EvidenceSnapshotRepository final {
public:
    struct Summary {
        std::int64_t id = 0;
        std::int64_t startTimestampMs = 0;
        std::int64_t endTimestampMs = 0;
        double totalEquity = 0.0;
        double returnRate = 0.0;
        double maxDrawdown = 0.0;
        std::string source;
        std::string priceBasis;
    };
    struct Detail {
        Summary summary;
        std::vector<simulation::Trade> trades;
        std::vector<analysis::BehaviorFinding> findings;
    };

    explicit EvidenceSnapshotRepository(Database& database);

    std::optional<std::int64_t> save(const analysis::EvidenceSnapshot& evidence,
                                     const analysis::BehaviorReport& report);
    std::int64_t count() const;
    std::vector<Summary> recent(int limit = 50) const;
    std::optional<Detail> load(std::int64_t snapshotId) const;
    bool remove(std::int64_t snapshotId);

private:
    Database& database_;
};

} // namespace fininsight::storage
