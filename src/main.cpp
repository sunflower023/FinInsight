#include "app/MainWindow.h"
#include "core/AppConfig.h"
#include "storage/Database.h"
#include "datahub/QuoteData.h"
#include "datahub/DataHub.h"

#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("FinInsight");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("FinInsight");

    // 注册自定义类型（QVariant 序列化需要）
    qRegisterMetaType<fininsight::datahub::QuoteData>("QuoteData");
    qRegisterMetaType<QVector<fininsight::datahub::KLineData>>("KLineDataVec");

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
