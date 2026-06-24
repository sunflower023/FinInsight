#pragma once

#include <QString>

class AppConfig
{
public:
    static AppConfig &instance();

    QString databasePath() const;
    QString cachePath() const;
    QString dataPath() const;

    void setDatabasePath(const QString &path);
    void setCachePath(const QString &path);

private:
    AppConfig() = default;
    QString db_path_;
    QString cache_path_;
};
