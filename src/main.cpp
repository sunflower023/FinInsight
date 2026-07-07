#include "app/MainWindow.h"
#include "core/AppConfig.h"
#include "storage/Database.h"

#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("FinInsight");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("FinInsight");

    // 初始化数据库（自动建库建表）
    const QString dbPath = AppConfig::instance().databasePath();
    if (fininsight::storage::Database::instance().open(dbPath)) {
        qInfo() << "Database initialized at:" << dbPath;
    } else {
        qWarning() << "Failed to open database:" << dbPath;
    }

    MainWindow window;
    window.resize(1280, 800);
    window.show();

    return app.exec();
}
