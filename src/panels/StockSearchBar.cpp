#include "panels/StockSearchBar.h"
#include "core/I18n.h"

#include <QRegularExpression>

namespace fininsight::panels {

StockSearchBar::StockSearchBar(QWidget* parent)
    : QLineEdit(parent)
    , debounceTimer_(new QTimer(this))
{
    setClearButtonEnabled(true);
    setMinimumHeight(40);
    setMaximumHeight(44);

    connect(this, &QLineEdit::returnPressed, this, &StockSearchBar::onReturnPressed);
    retranslateUi();
}

void StockSearchBar::retranslateUi() {
    setPlaceholderText(I18n::instance().t("Enter stock symbol and press Enter (e.g. AAPL, TSLA, 600519)"));
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
