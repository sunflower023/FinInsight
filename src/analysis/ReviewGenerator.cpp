#include "analysis/ReviewGenerator.h"

#include <sstream>

namespace fininsight::analysis {

ReviewResult DeterministicReviewGenerator::generate(const EvidenceSnapshot& evidence,
                                                     const BehaviorReport& report) const {
    ReviewResult result;
    std::ostringstream overview;
    overview << "Evidence covers " << evidence.trades.size() << " trades";
    if (evidence.portfolio.initialCash > 0.0) {
        overview << ", return rate " << (evidence.portfolio.returnRate * 100.0)
                 << " percent and maximum drawdown " << (evidence.maxDrawdown * 100.0) << " percent.";
    } else {
        overview << ".";
    }
    result.paragraphs.push_back(overview.str());
    for (const auto& finding : report.findings) {
        std::ostringstream paragraph;
        paragraph << "[" << finding.code << "] " << finding.message
                  << " Measure=" << finding.measure;
        if (!finding.evidenceTradeIds.empty()) {
            paragraph << ". Evidence trade IDs:";
            for (const auto id : finding.evidenceTradeIds) paragraph << ' ' << id;
        }
        result.paragraphs.push_back(paragraph.str());
        result.evidenceCodes.push_back(finding.code);
    }
    if (report.findings.empty()) {
        result.paragraphs.push_back("No configured behavior rule was triggered by the available evidence.");
    }
    return result;
}

} // namespace fininsight::analysis
