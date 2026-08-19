#pragma once

#include "analysis/BehaviorAnalyzer.h"

#include <QWidget>
#include <QVector>
#include <cstdint>
#include <vector>
#include <memory>

class QComboBox;
class QLabel;
class QListWidget;
class QTableWidget;
class QListWidgetItem;
class QPushButton;

namespace fininsight::storage { class EvidenceSnapshotRepository; }
namespace fininsight::analysis { class OpenAICompatibleReviewGenerator; }

namespace fininsight::panels {

class AgentReviewPanel final : public QWidget {
    Q_OBJECT
public:
    explicit AgentReviewPanel(storage::EvidenceSnapshotRepository& repository,
                              QWidget* parent = nullptr);
    ~AgentReviewPanel() override;
    void refresh();

private slots:
    void loadSelected(int index);
    void highlightFinding(QListWidgetItem* item);
    void generateModelReview();

private:
    void clearDetail();
    storage::EvidenceSnapshotRepository& repository_;
    QComboBox* snapshots_ = nullptr;
    QLabel* summary_ = nullptr;
    QLabel* review_ = nullptr;
    QLabel* modelStatus_ = nullptr;
    QPushButton* generateButton_ = nullptr;
    QPushButton* cancelButton_ = nullptr;
    QListWidget* findings_ = nullptr;
    QTableWidget* trades_ = nullptr;
    QVector<std::vector<std::uint64_t>> findingTradeIds_;
    std::unique_ptr<analysis::OpenAICompatibleReviewGenerator> modelGenerator_;
    analysis::EvidenceSnapshot currentEvidence_;
    analysis::BehaviorReport currentReport_;
    QString activeRequestId_;
};

} // namespace fininsight::panels
