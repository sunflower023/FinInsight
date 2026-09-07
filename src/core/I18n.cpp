#include "core/I18n.h"

#include <QSettings>

namespace {

/// 英文 → 中文 翻译表（源文本必须与代码中的字符串字面量完全一致）
QHash<QString, QString> buildTranslations()
{
    QHash<QString, QString> t;

    // ── 菜单栏 ────────────────────────────────
    t["File"] = "文件";
    t["Exit"] = "退出";
    t["View"] = "视图";
    t["Reset Layout"] = "重置布局";
    t["Data"] = "数据";
    t["Language"] = "语言";
    t["Experimental Realtime Quotes"] = "实验性实时行情";

    // ── Dock 面板标题 ─────────────────────────
    t["Watchlist"] = "自选股";
    t["Chart"] = "K线图";
    t["Detail"] = "详情";
    t["Simulation Portfolio"] = "模拟组合";
    t["Historical Experiment"] = "历史实验";
    t["Agent Review"] = "行为复盘";
    t["Realtime Paper Trading"] = "实时模拟交易";
    t["Notifications"] = "通知";
    t["Health"] = "健康监控";

    // ── 策略体检（DSL 表达式） ─────────────────
    t["Strategy Check"] = "策略体检";
    t["Check"] = "体检";
    t["Hit"] = "命中";
    t["Not Hit"] = "未命中";
    t["Load a stock first"] = "请先加载一只股票";
    t["Expression Error"] = "表达式错误";

    // ── 状态栏 / 实时流 ───────────────────────
    t["Loading..."] = "加载中...";
    t["Realtime: off"] = "实时：关闭";
    t["Realtime: connecting"] = "实时：连接中";
    t["Realtime: live (experimental)"] = "实时：已连接（实验）";
    t["Realtime: HTTP fallback"] = "实时：HTTP 回退";
    t["Realtime: stale / fallback"] = "实时：延迟/回退";
    t["Realtime: disconnected"] = "实时：已断开";

    // ── 模拟组合 PortfolioPanel ───────────────
    t["Cash"] = "现金";
    t["Holdings"] = "持仓市值";
    t["Total equity"] = "总权益";
    t["Realized P&L"] = "已实现盈亏";
    t["Unrealized P&L"] = "未实现盈亏";
    t["Total P&L"] = "总盈亏";
    t["No symbol selected"] = "未选择标的";
    t["Waiting for quote"] = "等待报价";
    t["Qty: "] = "数量：";
    t["Fee: $"] = "手续费：$";
    t["Buy"] = "买入";
    t["Sell"] = "卖出";
    t["Bought"] = "已买入";
    t["Sold"] = "已卖出";
    t["A valid quote is required before trading"] = "交易前需要有效报价";

    // ── 详情 DetailPanel ──────────────────────
    t["Field"] = "字段";
    t["Value"] = "值";
    t["Name"] = "名称";
    t["Change"] = "涨跌";
    t["Change %"] = "涨跌幅";
    t["Open"] = "开盘";
    t["High"] = "最高";
    t["Low"] = "最低";

    // ── 通用表头 ──────────────────────────────
    t["Symbol"] = "代码";
    t["Side"] = "方向";
    t["Qty"] = "数量";
    t["Price"] = "价格";
    t["Fee"] = "手续费";
    t["Time"] = "时间";
    t["Status"] = "状态";
    t["Type"] = "类型";
    t["Source"] = "来源";
    t["Event"] = "事件";
    t["Message"] = "消息";
    t["ID"] = "ID";
    t["Quantity"] = "数量";
    t["Limit"] = "限价";
    t["BUY"] = "买入";
    t["SELL"] = "卖出";
    t["MARKET"] = "市价";
    t["LIMIT"] = "限价";

    // ── 自选股 / 搜索框 ───────────────────────
    t["Remove"] = "移除";
    t["Enter stock symbol and press Enter (e.g. AAPL, TSLA, 600519)"] =
        "输入股票代码并回车（如 AAPL、TSLA、600519）";

    // ── 历史实验 ExperimentPanel ──────────────
    t["Select a symbol and wait for daily bars"] = "选择标的并等待日线数据";
    t["Run"] = "运行";
    t["Close"] = "收盘价";
    t["Adjusted close"] = "复权收盘价";
    t["From"] = "开始";
    t["To"] = "结束";
    t["Initial cash"] = "初始资金";
    t["Buy fee"] = "买入手续费";
    t["Action"] = "操作";
    t["Execution"] = "成交";
    t["Ending"] = "期末";
    t["Remaining cash"] = "剩余现金";
    t["Market value"] = "持仓市值";
    t["Ending equity"] = "期末权益";
    t["Return"] = "收益率";
    t["Max drawdown"] = "最大回撤";
    t["Historical daily bars are required"] = "需要历史日线数据";
    t["%1 | Waiting for daily bars"] = "%1 | 等待日线数据";
    t["%1 | Historical data unavailable"] = "%1 | 历史数据不可用";
    t["%1 | %2 daily bars | %3 to %4"] = "%1 | %2 根日线 | %3 至 %4";
    t["%1 at %2"] = "%1 成交价 %2";
    t["Experiment completed using %1"] = "实验完成，价格口径：%1";

    // ── 行为复盘 AgentReviewPanel ─────────────
    t["Generate Model Review"] = "生成模型复盘";
    t["Cancel"] = "取消";
    t["Deterministic offline review"] = "确定性离线复盘";
    t["No saved evidence snapshot"] = "暂无保存的证据快照";
    t["No deterministic review available"] = "暂无可用的确定性复盘";
    t["Cancelled; deterministic review retained"] = "已取消；保留确定性复盘";
    t["Model-generated review"] = "模型生成的复盘";
    t["Model unavailable: %1; deterministic review retained"] =
        "模型不可用：%1；保留确定性复盘";
    t["Model configured; deterministic review shown"] = "模型已配置；显示确定性复盘";
    t["No model key; deterministic offline review"] = "无模型密钥；确定性离线复盘";
    t["Generating model review..."] = "正在生成模型复盘...";

    // ── 实时模拟交易 RealtimeTradingPanel ─────
    t["PAPER"] = "模拟";
    t["No quote"] = "无报价";
    t["No current quote"] = "无当前报价";
    t["Submit Paper Order"] = "提交模拟订单";
    t["Cancel Selected"] = "取消选中";
    t["Kill switch"] = "紧急熔断";
    t["Order cancelled"] = "订单已取消";
    t["Order cannot be cancelled"] = "订单无法取消";
    t["Cash: $%1 | Equity: $%2 | %3 position: %4"] =
        "现金：$%1 | 权益：$%2 | %3 持仓：%4";

    // ── 通知 NotificationPanel ────────────────
    t["Mark All Read"] = "全部标为已读";
    t["Unread: %1"] = "未读：%1";

    // ── 健康监控 HealthPanel ──────────────────
    t["Disabled"] = "已禁用";
    t["Healthy"] = "健康";
    t["Degraded"] = "降级";
    t["Stale"] = "延迟";
    t["Down"] = "宕机";
    t["never"] = "从未";
    t["Quote stream: %1 | Last tick: %2"] = "行情流：%1 | 最近心跳：%2";
    t["Fallbacks: %1 | Disconnects: %2 | Rejected messages: %3"] =
        "回退：%1 | 断开：%2 | 拒绝消息：%3";
    t["Quote delivery latency (%1): P50 %2 ms | P95 %3 ms | P99 %4 ms"] =
        "报价送达延迟（%1）：P50 %2 ms | P95 %3 ms | P99 %4 ms";

    return t;
}

} // namespace

I18n &I18n::instance()
{
    static I18n instance;
    return instance;
}

I18n::I18n()
    : translations_(buildTranslations())
{
    // 恢复上次选择的语言
    QSettings settings;
    const QString saved = settings.value("language").toString();
    if (saved == "zh") {
        language_ = Language::Chinese;
    }
}

void I18n::setLanguage(Language language)
{
    if (language_ == language) return;
    language_ = language;
    QSettings settings;
    settings.setValue("language", language == Language::Chinese ? "zh" : "en");
    emit languageChanged();
}

QString I18n::t(const QString &english) const
{
    if (language_ == Language::English) return english;
    return translations_.value(english, english);
}
