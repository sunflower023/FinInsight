#include "panels/AgentReviewPanel.h"

#include "storage/EvidenceSnapshotRepository.h"
#include "analysis/ReviewGenerator.h"
#include "analysis/OpenAICompatibleReviewGenerator.h"

#include <QComboBox>
#include <QDateTime>
#include <QFormLayout>
#include <QLabel>
#include <QListWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QPushButton>
#include <QHBoxLayout>
#include <QProcessEnvironment>
#include <algorithm>

namespace fininsight::panels {

AgentReviewPanel::AgentReviewPanel(storage::EvidenceSnapshotRepository& repository,
                                   QWidget* parent)
    : QWidget(parent), repository_(repository) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 6, 8, 6);
    snapshots_ = new QComboBox;
    snapshots_->setObjectName(QStringLiteral("agentSnapshotSelector"));
    layout->addWidget(snapshots_);
    summary_ = new QLabel;
    summary_->setObjectName(QStringLiteral("agentSummaryLabel"));
    summary_->setWordWrap(true);
    layout->addWidget(summary_);
    review_ = new QLabel;
    review_->setObjectName(QStringLiteral("agentReviewTextLabel"));
    review_->setWordWrap(true);
    layout->addWidget(review_);
    auto* modelRow = new QHBoxLayout;
    generateButton_ = new QPushButton(QStringLiteral("Generate Model Review"));
    generateButton_->setObjectName(QStringLiteral("agentGenerateButton"));
    cancelButton_ = new QPushButton(QStringLiteral("Cancel")); cancelButton_->setObjectName(QStringLiteral("agentCancelButton")); cancelButton_->setEnabled(false);
    modelStatus_ = new QLabel(QStringLiteral("Deterministic offline review")); modelStatus_->setObjectName(QStringLiteral("agentModelStatusLabel"));
    modelRow->addWidget(generateButton_); modelRow->addWidget(cancelButton_); modelRow->addWidget(modelStatus_, 1); layout->addLayout(modelRow);
    findings_ = new QListWidget;
    findings_->setObjectName(QStringLiteral("agentFindingsList"));
    layout->addWidget(findings_);
    trades_ = new QTableWidget(0, 5);
    trades_->setObjectName(QStringLiteral("agentTradesTable"));
    trades_->setHorizontalHeaderLabels({QStringLiteral("Side"), QStringLiteral("Symbol"),
        QStringLiteral("Qty"), QStringLiteral("Price"), QStringLiteral("Time")});
    trades_->horizontalHeader()->setStretchLastSection(true);
    trades_->verticalHeader()->setVisible(false);
    trades_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(trades_, 1);
    connect(snapshots_, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &AgentReviewPanel::loadSelected);
    connect(findings_, &QListWidget::itemClicked,
            this, &AgentReviewPanel::highlightFinding);
    const auto env = QProcessEnvironment::systemEnvironment();
    analysis::OpenAICompatibleReviewGenerator::Config config;
    config.endpoint = env.value(QStringLiteral("FININSIGHT_LLM_ENDPOINT"), QStringLiteral("https://api.openai.com/v1/chat/completions"));
    config.apiKey = env.value(QStringLiteral("FININSIGHT_LLM_API_KEY")); config.model = env.value(QStringLiteral("FININSIGHT_LLM_MODEL"), QStringLiteral("gpt-4.1-mini"));
    modelGenerator_ = std::make_unique<analysis::OpenAICompatibleReviewGenerator>(config, this);
    connect(generateButton_, &QPushButton::clicked, this, &AgentReviewPanel::generateModelReview);
    connect(cancelButton_, &QPushButton::clicked, this, [this] { modelGenerator_->cancel(); activeRequestId_.clear(); cancelButton_->setEnabled(false); generateButton_->setEnabled(true); modelStatus_->setText(QStringLiteral("Cancelled; deterministic review retained")); });
    connect(modelGenerator_.get(), &analysis::OpenAICompatibleReviewGenerator::completed, this, [this](const QString& id, const QString& text) {
        if (id != activeRequestId_) return; activeRequestId_.clear(); review_->setText(text); modelStatus_->setText(QStringLiteral("Model-generated review")); cancelButton_->setEnabled(false); generateButton_->setEnabled(true); });
    connect(modelGenerator_.get(), &analysis::OpenAICompatibleReviewGenerator::failed, this, [this](const QString& id, const QString& error) {
        if (id != activeRequestId_) return; activeRequestId_.clear(); modelStatus_->setText(QStringLiteral("Model unavailable: %1; deterministic review retained").arg(error)); cancelButton_->setEnabled(false); generateButton_->setEnabled(true); });
    clearDetail();
    refresh();
}
AgentReviewPanel::~AgentReviewPanel() = default;

void AgentReviewPanel::refresh() {
    const auto previous = snapshots_->currentData().toLongLong();
    snapshots_->blockSignals(true);
    snapshots_->clear();
    for (const auto& item : repository_.recent()) {
        snapshots_->addItem(QStringLiteral("#%1 | %2 | %3% | %4")
            .arg(item.id)
            .arg(QString::fromStdString(item.source))
            .arg(item.returnRate * 100.0, 0, 'f', 2)
            .arg(QString::fromStdString(item.priceBasis)),
            QVariant::fromValue(qlonglong(item.id)));
    }
    int selected = snapshots_->findData(QVariant::fromValue(qlonglong(previous)));
    if (selected < 0 && snapshots_->count() > 0) selected = 0;
    snapshots_->setCurrentIndex(selected);
    snapshots_->blockSignals(false);
    loadSelected(selected);
}

void AgentReviewPanel::loadSelected(int index) {
    if (modelGenerator_) modelGenerator_->cancel();
    activeRequestId_.clear();
    clearDetail();
    if (index < 0) return;
    const auto detail = repository_.load(snapshots_->itemData(index).toLongLong());
    if (!detail) return;
    const auto& s = detail->summary;
    summary_->setText(QStringLiteral("Source: %1 | Price: %2 | Equity: $%3 | Return: %4% | Max drawdown: %5%")
        .arg(QString::fromStdString(s.source), QString::fromStdString(s.priceBasis))
        .arg(s.totalEquity, 0, 'f', 2)
        .arg(s.returnRate * 100.0, 0, 'f', 2)
        .arg(s.maxDrawdown * 100.0, 0, 'f', 2));
    analysis::EvidenceSnapshot evidence;
    evidence.startTimestampMs = s.startTimestampMs;
    evidence.endTimestampMs = s.endTimestampMs;
    evidence.maxDrawdown = s.maxDrawdown;
    evidence.portfolio.totalEquity = s.totalEquity;
    evidence.portfolio.returnRate = s.returnRate;
    evidence.trades = detail->trades;
    analysis::BehaviorReport report;
    report.findings = detail->findings;
    const auto review = analysis::DeterministicReviewGenerator().generate(evidence, report);
    QStringList paragraphs;
    for (const auto& paragraph : review.paragraphs)
        paragraphs.push_back(QString::fromStdString(paragraph));
    review_->setText(paragraphs.join(QStringLiteral("\n")));
    currentEvidence_ = evidence; currentReport_ = report;
    modelStatus_->setText(modelGenerator_ && modelGenerator_->isConfigured() ? QStringLiteral("Model configured; deterministic review shown") : QStringLiteral("No model key; deterministic offline review"));
    generateButton_->setEnabled(true); cancelButton_->setEnabled(false);
    findingTradeIds_.clear();
    for (const auto& finding : detail->findings) {
        findings_->addItem(QStringLiteral("[%1] %2: %3")
            .arg(QString::fromStdString(finding.code))
            .arg(finding.severity == analysis::FindingSeverity::Warning ? QStringLiteral("warning") : QStringLiteral("info"))
            .arg(QString::fromStdString(finding.message)));
        findingTradeIds_.push_back(finding.evidenceTradeIds);
    }
    for (const auto& trade : detail->trades) {
        const int row = trades_->rowCount();
        trades_->insertRow(row);
        trades_->setItem(row, 0, new QTableWidgetItem(trade.side == simulation::TradeSide::Buy ? "BUY" : "SELL"));
        trades_->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(trade.symbol)));
        trades_->setItem(row, 2, new QTableWidgetItem(QString::number(trade.quantity)));
        trades_->setItem(row, 3, new QTableWidgetItem(QString::number(trade.price, 'f', 2)));
        trades_->setItem(row, 4, new QTableWidgetItem(QDateTime::fromMSecsSinceEpoch(trade.timestampMs).toString(Qt::ISODate)));
        trades_->item(row, 0)->setData(Qt::UserRole, qulonglong(trade.id));
    }
}
void AgentReviewPanel::generateModelReview()
{
    if (!modelGenerator_ || currentEvidence_.trades.empty()) return;
    activeRequestId_ = modelGenerator_->generateAsync(currentEvidence_, currentReport_);
    generateButton_->setEnabled(false); cancelButton_->setEnabled(true); modelStatus_->setText(QStringLiteral("Generating model review..."));
}

void AgentReviewPanel::highlightFinding(QListWidgetItem* item) {
    const int index = findings_->row(item);
    if (index < 0 || index >= findingTradeIds_.size()) return;
    trades_->clearSelection();
    const auto& ids = findingTradeIds_[index];
    for (int row = 0; row < trades_->rowCount(); ++row) {
        const auto id = trades_->item(row, 0)->data(Qt::UserRole).toULongLong();
        if (std::find(ids.begin(), ids.end(), id) != ids.end()) trades_->selectRow(row);
    }
}

void AgentReviewPanel::clearDetail() {
    summary_->setText(QStringLiteral("No saved evidence snapshot"));
    review_->setText(QStringLiteral("No deterministic review available"));
    findings_->clear();
    findingTradeIds_.clear();
    trades_->setRowCount(0);
    currentEvidence_ = {}; currentReport_ = {};
}

} // namespace fininsight::panels
