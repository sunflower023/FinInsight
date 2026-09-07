#include "app/MainWindow.h"
#include "core/AppConfig.h"
#include "storage/Database.h"
#include "datahub/QuoteData.h"
#include "datahub/DataHub.h"

#include <QApplication>
#include <QPalette>
#include <QStyleFactory>
#include <QDebug>

// 构造固定的浅色调色板。项目 UI 采用浅色设计（见 MainWindow 的 kGlobalStyle），
// 若不显式设置 palette，Windows 深色模式下未被 QSS 覆盖的控件(ComboBox 下拉、
// 滚动视图、Qt ADS 容器等)会回落到系统深色 → 出现难看的黑色图块。
// Fusion 风格 + 固定浅色 palette 可让配色完全自洽，不再随系统主题变化。
static void applyLightTheme(QApplication& app)
{
    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette p;

    // 常规组：Google Material 浅色，与 kGlobalStyle 一致
    const QColor window("#ffffff");      // 窗口/面板底
    const QColor windowText("#1e1e1e");  // 主文字
    const QColor base("#ffffff");        // 输入框/表格底
    const QColor altBase("#f8f9fa");     // 斑马纹
    const QColor text("#1e1e1e");
    const QColor button("#ffffff");      // 按钮底
    const QColor buttonText("#1e1e1e");
    const QColor highlight("#1a73e8");   // 选中高亮(品牌蓝)
    const QColor highlightedText("#ffffff");
    const QColor placeholder("#9aa0a6"); // 占位符
    const QColor disabledText("#b0b3b8");
    const QColor tooltipBase("#fdfcff");
    const QColor tooltipText("#1e1e1e");

    p.setColor(QPalette::Window, window);
    p.setColor(QPalette::WindowText, windowText);
    p.setColor(QPalette::Base, base);
    p.setColor(QPalette::AlternateBase, altBase);
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::Button, button);
    p.setColor(QPalette::ButtonText, buttonText);
    p.setColor(QPalette::Highlight, highlight);
    p.setColor(QPalette::HighlightedText, highlightedText);
    p.setColor(QPalette::PlaceholderText, placeholder);
    p.setColor(QPalette::ToolTipBase, tooltipBase);
    p.setColor(QPalette::ToolTipText, tooltipText);
    p.setColor(QPalette::Link, highlight);
    p.setColor(QPalette::LinkVisited, highlight);
    p.setColor(QPalette::BrightText, Qt::red);

    // 禁用态文字(置灰、不可用控件)
    p.setColor(QPalette::Disabled, QPalette::WindowText, disabledText);
    p.setColor(QPalette::Disabled, QPalette::Text, disabledText);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, disabledText);

    app.setPalette(p);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("FinInsight");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("FinInsight");

    // 注册自定义类型（QVariant 序列化需要）
    qRegisterMetaType<fininsight::datahub::QuoteData>("QuoteData");
    qRegisterMetaType<QVector<fininsight::datahub::KLineData>>("KLineDataVec");

    // 强制浅色主题：避免控件随 Windows 深色模式变黑
    applyLightTheme(app);

    // —— 初始化数据库 ——
    const QString dbPath = AppConfig::instance().databasePath();
    if (fininsight::storage::Database::instance().open(dbPath)) {
        qInfo() << "Database initialized at:" << dbPath;
    } else {
        qWarning() << "Failed to open database:" << dbPath;
    }

    // —— 启动 UI ——
    MainWindow window;
    window.resize(1280, 800);
    window.show();

    return app.exec();
}
