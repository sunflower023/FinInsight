#include "panels/DetailPanel.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QDateTime>

namespace fininsight::panels {

DetailPanel::DetailPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    auto* title = new QLabel("Detail");
    title->setStyleSheet("font-weight:600; font-size:13px; color:#5f6368;"
                         "letter-spacing:0.3px; padding:0 0 6px 0;");
    layout->addWidget(title);

    table_ = new QTableWidget(8, 2);
    table_->setHorizontalHeaderLabels({"Field", "Value"});
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->verticalHeader()->setVisible(false);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setAlternatingRowColors(true);
    table_->setShowGrid(false);

    QStringList fields = {"Symbol", "Name", "Price", "Change",
                          "Change %", "Open", "High", "Low"};
    for (int i = 0; i < fields.size(); ++i) {
        auto* item = new QTableWidgetItem(fields[i]);
        item->setForeground(QColor("#5f6368"));
        table_->setItem(i, 0, item);
    }

    layout->addWidget(table_);
}

void DetailPanel::updateQuote(const datahub::QuoteData& quote) {
    if (!table_) return;

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

} // namespace fininsight::panels
