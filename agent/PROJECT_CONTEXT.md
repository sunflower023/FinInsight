# Project Context

## 当前事实

- 语言与框架：C++20、Qt6 Widgets/Charts/Network/Sql/Concurrent、SQLite、CMake。
- 代码规模：包含 Qt 桌面主工程、纯 C++ 核心和独立测试；具体规模随开发变化，不以旧行数作为完成度依据。
- 当前主链路：`MainWindow -> YahooProducer -> HttpClient 异步请求 -> Yahoo API -> DataHub -> panels/charts`。
- SQLite 当前缓存股票基础信息和报价；V002 已增加 evidence snapshots、交易明细和行为 findings，Portfolio/实验主状态仍未完整持久化。
- DSL、指标引擎、多源聚合器和模拟核心属于可复用模块，但并非全部接入主 UI。

## 产品方向：模拟投资实验室

FinInsight 将把模拟投资作为面向金融初学者的核心业务闭环：用户以虚拟资金基于历史或当前行情做出决策，观察结果，并通过可解释的复盘理解风险与行为。它不是荐股工具，也不承诺或暗示真实投资收益。

- 核心问题：在没有真实资金的前提下，为学习者提供“决策 - 结果 - 复盘”的反馈。
- 首个高价值场景：选择历史时点、标的、金额和结束日期，查看至结束日期的收益、最大回撤及相对基准表现。纯 C++ 账本、单标的买入持有实验、历史价格转换和 Qt 实验面板已经完成；结果尚未持久化。
- 差异化方向：从“模拟炒股”扩展为可复现的投资决策实验和行为复盘，而非只显示当前浮盈浮亏。
- 后续 AI 能力仅分析用户的模拟交易与风险行为，输出可追溯的证据和教育性建议；不得直接生成确定性买卖指令。

## 诚实的完成度口径

| 能力 | 状态 |
|---|---|
| Qt Dock 工作台 | 已接入 |
| Yahoo 报价/K线 | 已接入，主链路异步 |
| DataHub 订阅与回放 | 已接入 |
| SQLite 基础缓存 | 已接入 |
| 技术指标计算 | 独立模块，部分接入图表 |
| EastMoney/Sina | 异步 Producer / 共享适配器与聚合原型 |
| 多源 first-valid 竞速 | 异步原型 | 已有路由、领域校验、超时和取消；离线状态机测试已覆盖，尚未接入 MainWindow |
| DSL 策略回测 | 未接入 |
| 组合交易界面 | 内存闭环已完成 | PortfolioPanel 使用 Ledger，支持报价联动、买卖、手续费、交易拒绝、持仓估值和盈亏展示；尚未持久化 |
| 模拟投资账本 | 已接入 UI | 纯 C++ 买卖、持仓、手续费、已实现/未实现盈亏和估值；MainWindow/Portfolio 已接入，SQLite 尚未接入 |
| 历史买入持有实验 | Qt 闭环已完成 | 单标的成交、期末估值、收益率和最大回撤；已由实验面板调用，SQLite 尚未接入 |
| 历史价格转换 | Qt 闭环已完成 | ISO 日期、严格顺序、普通/复权收盘价校验；已接入 MainWindow K 线回调 |
| WebSocket 实时行情 | Yahoo 实验适配初版 | Yahoo streamer Protobuf ticker 已转换为 `QuoteData` 并发布到 `<symbol>.quote.realtime`；尚未接入 MainWindow 股票主链路 |
| 产品内 AI Agent | 确定性分析基础已实现 | `src/analysis/BehaviorAnalyzer` 输出交易频率、集中度、盈亏不对称和回撤期交易证据；LLM、SQLite 证据快照和 Agent 面板尚未实现 |

## 当前主要未完成项

1. Portfolio 交易、持仓和估值仍为内存状态，应用重启后会重置。
2. 历史实验结果仍仅保存在内存中，尚未接入 SQLite 快照和历史结果列表。
3. Aggregator 尚未替换当前 YahooProducer 主链路，缓存降级策略仍未实现。
4. 模拟交易、实验参数、价格口径和结果尚未迁移到 SQLite。
5. 基准比较、风险扩展指标、组合层能力和 DSL 回测尚未实现。
6. WebSocket 尚未接入股票主链路；产品内 Agent 目前只有确定性分析基础，尚未持久化或接入 LLM/UI。

## 优先风险

1. 旧同步 `HttpClient::get()` 仅作为兼容接口保留，Aggregator 已改用异步模型。
2. Aggregator 尚未接入 MainWindow，缓存降级和运行时验证仍缺失。
3. 指标算法、Parser、K 线 JSON 解析和 Qt 历史实验桥接仍缺少完整自动化测试；报价解析、异步 HTTP、Aggregator 和纯 C++ 模拟核心已有离线 CTest。
4. CMake 预设包含本机 Qt 路径，跨机器构建前必须改为用户配置或工具链参数。
