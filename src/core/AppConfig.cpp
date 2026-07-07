#include "AppConfig.h"

#include <QStandardPaths>
#include <QDir>

AppConfig &AppConfig::instance()
{
    static AppConfig config;
    return config;
}

QString AppConfig::databasePath() const
{
    if (!db_path_.isEmpty())
        return db_path_;

    // 使用 GenericDataLocation 获取 %LOCALAPPDATA%，
    // 比 AppLocalDataLocation 更可靠（不依赖 QApplication 命名）。
    const QString base = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    const QString dir  = base + "/FinInsight";
    QDir().mkpath(dir);
    return dir + "/fininsight.db";
}

QString AppConfig::cachePath() const
{
    if (!cache_path_.isEmpty())
        return cache_path_;

    const QString base = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    const QString dir  = base + "/FinInsight/cache";
    QDir().mkpath(dir);
    return dir;
}

QString AppConfig::dataPath() const
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    return base + "/FinInsight";
}

void AppConfig::setDatabasePath(const QString &path)
{
    db_path_ = path;
}

void AppConfig::setCachePath(const QString &path)
{
    cache_path_ = path;
}
