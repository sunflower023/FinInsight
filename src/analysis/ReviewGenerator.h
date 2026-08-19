#pragma once

#include "analysis/BehaviorAnalyzer.h"

#include <string>
#include <vector>

namespace fininsight::analysis {

struct ReviewResult {
    std::vector<std::string> paragraphs;
    std::vector<std::string> evidenceCodes;
};

class ReviewGenerator {
public:
    virtual ~ReviewGenerator() = default;
    virtual ReviewResult generate(const EvidenceSnapshot& evidence,
                                  const BehaviorReport& report) const = 0;
};

class DeterministicReviewGenerator final : public ReviewGenerator {
public:
    ReviewResult generate(const EvidenceSnapshot& evidence,
                          const BehaviorReport& report) const override;
};

} // namespace fininsight::analysis
