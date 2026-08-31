#pragma once

#include <QHash>
#include <QObject>
#include <QString>

/**
 * @brief 全局中英文切换的单例
 *
 * 设计：
 *   - 以英文为源文本，中文译文存于内置翻译表；
 *   - 未命中的键原样返回英文，保证英文模式与切换失败时都不会丢字。
 *
 * 用法：
 *   I18n::instance().t("Buy")                              // 取当前语言文本
 *   I18n::instance().setLanguage(I18n::Language::Chinese)   // 切换并广播
 *   connect(&I18n::instance(), &I18n::languageChanged, ...) // 运行时刷新界面
 */
class I18n : public QObject
{
    Q_OBJECT

public:
    enum class Language { English, Chinese };

    static I18n &instance();

    Language language() const { return language_; }
    bool isChinese() const { return language_ == Language::Chinese; }

    /// 切换语言并广播 languageChanged 信号（用于刷新界面）
    void setLanguage(Language language);

    /// 翻译：中文返回译文，英文或未命中返回原文
    /// 注意：命名为 t() 而非 tr()，避免与 QObject::tr(const char*) 重载冲突
    /// （const char* 字面量会被 QObject::tr 优先匹配，导致译文失效）
    QString t(const QString &english) const;

signals:
    void languageChanged();

private:
    I18n();
    Language language_ = Language::English;
    QHash<QString, QString> translations_;
};
