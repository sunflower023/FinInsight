#include "panels/StockSearchBar.h"

#include <QRegularExpression>

namespace fininsight::panels {

StockSearchBar::StockSearchBar(QWidget* parent)
    : QLineEdit(parent)
    , debounceTimer_(new QTimer(this))
{
    setPlaceholderText("Enter stock symbol and press Enter (e.g. AAPL, TSLA, 600519)");
    setClearButtonEnabled(true);
    setMinimumHeight(40);
    setMaximumHeight(44);

    connect(this, &QLineEdit::returnPressed, this, &StockSearchBar::onReturnPressed);
}

void StockSearchBar::onReturnPressed() {
    QString text = this->text().trimmed().toUpper();
    if (text.isEmpty()) return;

    // 只允许字母和数字，长度 1-6
    QRegularExpression re("^[A-Z0-9]{1,6}$");
    if (!re.match(text).hasMatch()) {
        setStyleSheet("background-color: #ffeeee;");
        return;
    }
    setStyleSheet("");
    emit searchRequested(text);
}

void StockSearchBar::onTextChanged(const QString& text) {
    Q_UNUSED(text);
}

void StockSearchBar::onDebounceTimeout() {
    // 不再自动搜索
}

} // namespace fininsight::panels
