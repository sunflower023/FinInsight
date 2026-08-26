#pragma once

#include <cstddef>
#include <deque>
#include <string>
#include <unordered_map>

namespace fininsight::monitoring {

struct LatencySummary {
    std::size_t sampleCount = 0;
    double p50Ms = 0.0;
    double p95Ms = 0.0;
    double p99Ms = 0.0;
    double maxMs = 0.0;
};

class LatencyTracker final {
public:
    explicit LatencyTracker(std::size_t capacityPerMetric = 256);
    bool record(const std::string& metric, double milliseconds);
    LatencySummary summary(const std::string& metric) const;
    std::size_t capacityPerMetric() const { return capacityPerMetric_; }

private:
    std::size_t capacityPerMetric_;
    std::unordered_map<std::string, std::deque<double>> samples_;
};

} // namespace fininsight::monitoring
