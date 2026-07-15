#include "panels/StockListPanel.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>

namespace fininsight::panels {

StockListPanel::StockListPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    auto* title = new QLabel("Watchlist");
    title->setStyleSheet("font-weight:600; font-size:13px; color:#5f6368;"
                         "letter-spacing:0.3px; padding:0 0 6px 0;");
    layout->addWidget(title);

    listWidget_ = new QListWidget();
    listWidget_->setAlternatingRowColors(true);
    layout->addWidget(listWidget_);

    btnRemove_ = new QPushButton("Remove");
    layout->addWidget(btnRemove_);

    connect(listWidget_, &QListWidget::itemDoubleClicked,
            this, &StockListPanel::onItemDoubleClicked);
    connect(btnRemove_, &QPushButton::clicked,
            this, &StockListPanel::onRemoveClicked);
}

void StockListPanel::addStock(const QString& symbol, const QString& name) {
    // 检查是否已存在
    for (int i = 0; i < listWidget_->count(); ++i) {
        if (listWidget_->item(i)->text().startsWith(symbol)) return;
    }
    listWidget_->addItem(QString("%1  %2").arg(symbol, -8).arg(name));
}

void StockListPanel::updatePrice(const QString& symbol, double price,
                                  double changePercent) {
    for (int i = 0; i < listWidget_->count(); ++i) {
        auto* item = listWidget_->item(i);
        if (item->text().startsWith(symbol)) {
            QColor color = changePercent >= 0 ? QColor("#d93025")
                                               : QColor("#188038");
            item->setForeground(color);
            item->setText(QString("%1  $%2 (%3%)")
                .arg(symbol, -8)
                .arg(price, 0, 'f', 2)
                .arg(changePercent, 0, 'f', 2));
            break;
        }
    }
}

void StockListPanel::onItemDoubleClicked(QListWidgetItem* item) {
    QString symbol = item->text().split(' ').first();
    emit stockSelected(symbol);
}

void StockListPanel::onRemoveClicked() {
    auto* item = listWidget_->currentItem();
    if (item) delete listWidget_->takeItem(listWidget_->row(item));
}

} // namespace fininsight::panels
