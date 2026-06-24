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

    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dir);
    return dir + "/fininsight.db";
}

QString AppConfig::cachePath() const
{
    if (!cache_path_.isEmpty())
        return cache_path_;

    const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir().mkpath(dir);
    return dir;
}

QString AppConfig::dataPath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
}

void AppConfig::setDatabasePath(const QString &path)
{
    db_path_ = path;
}

void AppConfig::setCachePath(const QString &path)
{
    cache_path_ = path;
}
