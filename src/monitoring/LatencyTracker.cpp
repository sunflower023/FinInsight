#include "monitoring/LatencyTracker.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace fininsight::monitoring {
namespace {
double percentile(const std::vector<double>& sorted, double probability)
{
    if (sorted.empty()) return 0.0;
    const auto index = static_cast<std::size_t>(std::ceil(probability * sorted.size())) - 1;
    return sorted[std::min(index, sorted.size() - 1)];
}
}

LatencyTracker::LatencyTracker(std::size_t capacityPerMetric) : capacityPerMetric_(capacityPerMetric)
{
    if (capacityPerMetric_ == 0) throw std::invalid_argument("latency capacity must be positive");
}

bool LatencyTracker::record(const std::string& metric, double milliseconds)
{
    if (metric.empty() || !std::isfinite(milliseconds) || milliseconds < 0.0) return false;
    auto& values = samples_[metric];
    values.push_back(milliseconds);
    while (values.size() > capacityPerMetric_) values.pop_front();
    return true;
}

LatencySummary LatencyTracker::summary(const std::string& metric) const
{
    const auto found = samples_.find(metric);
    if (found == samples_.end() || found->second.empty()) return {};
    std::vector<double> sorted(found->second.cbegin(), found->second.cend());
    std::sort(sorted.begin(), sorted.end());
    return {sorted.size(), percentile(sorted, 0.50), percentile(sorted, 0.95),
            percentile(sorted, 0.99), sorted.back()};
}

} // namespace fininsight::monitoring
