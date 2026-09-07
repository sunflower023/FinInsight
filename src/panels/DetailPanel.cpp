#include "panels/DetailPanel.h"
#include "core/I18n.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QGroupBox>
#include <QHeaderView>
#include <QDateTime>

namespace fininsight::panels {

DetailPanel::DetailPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    titleLabel_ = new QLabel;
    titleLabel_->setStyleSheet("font-weight:600; font-size:13px; color:#5f6368;"
                         "letter-spacing:0.3px; padding:0 0 6px 0;");
    layout->addWidget(titleLabel_);

    table_ = new QTableWidget(8, 2);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->verticalHeader()->setVisible(false);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setAlternatingRowColors(true);
    table_->setShowGrid(false);

    // 预先创建 8 行左侧字段单元格（文本由 retranslateUi 填充）
    for (int i = 0; i < 8; ++i) {
        auto* item = new QTableWidgetItem();
        item->setForeground(QColor("#5f6368"));
        table_->setItem(i, 0, item);
    }

    layout->addWidget(table_);

    // —— 策略体检（DSL 表达式） ——
    checkGroup_ = new QGroupBox;
    auto* checkLayout = new QVBoxLayout(checkGroup_);
    checkLayout->setContentsMargins(8, 8, 8, 8);
    checkLayout->setSpacing(6);

    checkEdit_ = new QLineEdit;
    checkEdit_->setPlaceholderText("MACD > 0 AND RSI(14) < 30");
    checkLayout->addWidget(checkEdit_);

    btnCheck_ = new QPushButton;
    btnCheck_->setCursor(Qt::PointingHandCursor);
    checkLayout->addWidget(btnCheck_);

    checkResult_ = new QLabel;
    checkResult_->setWordWrap(true);
    checkResult_->setStyleSheet("color:#5f6368; font-size:12px;");
    checkResult_->setText("—");
    checkLayout->addWidget(checkResult_);

    layout->addWidget(checkGroup_);
    layout->addStretch(1);

    connect(btnCheck_, &QPushButton::clicked,
            this, &DetailPanel::onCheckClicked);
    connect(checkEdit_, &QLineEdit::returnPressed,
            this, &DetailPanel::onCheckClicked);

    retranslateUi();
}

void DetailPanel::retranslateUi()
{
    titleLabel_->setText(I18n::instance().t("Detail"));
    table_->setHorizontalHeaderLabels({I18n::instance().t("Field"),
                                       I18n::instance().t("Value")});

    if (checkGroup_) {
        checkGroup_->setTitle(I18n::instance().t("Strategy Check"));
        btnCheck_->setText(I18n::instance().t("Check"));
    }

    const QStringList fields = {
        I18n::instance().t("Symbol"), I18n::instance().t("Name"),
        I18n::instance().t("Price"), I18n::instance().t("Change"),
        I18n::instance().t("Change %"), I18n::instance().t("Open"),
        I18n::instance().t("High"), I18n::instance().t("Low")};
    for (int i = 0; i < fields.size(); ++i) {
        if (auto* item = table_->item(i, 0)) item->setText(fields[i]);
    }
}

void DetailPanel::updateQuote(const datahub::QuoteData& quote) {
    if (!table_) return;
    currentSymbol_ = quote.symbol;

    auto setRow = [&](int row, const QString& val, const QColor& color = Qt::black) {
        auto* item = table_->item(row, 1);
        if (!item) {
            item = new QTableWidgetItem();
            table_->setItem(row, 1, item);
        }
        item->setText(val);
        item->setForeground(color);
    };

    setRow(0, quote.symbol);
    setRow(1, quote.name.isEmpty() ? quote.symbol : quote.name);
    setRow(2, QString::number(quote.price, 'f', 2),
           quote.change >= 0 ? QColor("#d93025") : QColor("#188038"));
    setRow(3, QString::number(quote.change, 'f', 2));
    setRow(4, QString::number(quote.changePercent, 'f', 2) + "%");
    setRow(5, QString::number(quote.open, 'f', 2));
    setRow(6, QString::number(quote.high, 'f', 2));
    setRow(7, QString::number(quote.low, 'f', 2));
}

void DetailPanel::clear() {
    for (int i = 0; i < 8; ++i) {
        auto* item = table_->item(i, 1);
        if (item) item->setText("-");
    }
}

void DetailPanel::onCheckClicked() {
    const QString expr = checkEdit_->text().trimmed();
    if (!expr.isEmpty()) emit strategyCheckRequested(expr);
}

void DetailPanel::setCheckResult(bool ok, bool hit, const QString& text) {
    if (!checkResult_) return;
    if (!ok) {
        checkResult_->setStyleSheet("color:#c5221f; font-size:12px;");
        checkResult_->setText(text);
        return;
    }
    // ok：区分「布尔条件命中」与「纯数值表达式」
    checkResult_->setStyleSheet(
        hit ? "color:#188038; font-size:12px;"
            : "color:#5f6368; font-size:12px;");
    QString prefix = text.isEmpty()
        ? (hit ? I18n::instance().t("Hit") : I18n::instance().t("Not Hit"))
        : text;
    checkResult_->setText(prefix);
}

} // namespace fininsight::panels
