#pragma once

#include <QSqlDatabase>
#include <functional>

/**
 * @brief 数据库迁移框架：每增加一个版本只需在 registerMigrations() 中注册
 *
 * 使用方式：
 *   MigrationRunner runner;
 *   runner.registerMigration({1, "Initial schema", V001_Initial::up});
 *   runner.apply(database);
 */
namespace fininsight::storage::migrations {

struct Migration {
    int version;                                     // 版本号（递增）
    const char* description;                         // 简短描述
    std::function<void(QSqlDatabase)> up;            // 升级回调 (QSqlDatabase 隐式共享)
};

} // namespace fininsight::storage::migrations
