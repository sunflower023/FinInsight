# Project Context

## 当前事实

- 语言与框架：C++20、Qt6 Widgets/Charts/Network/Sql/Concurrent、SQLite、CMake。
- 代码规模：约 42 个 `.cpp/.h` 文件，约 3.9k 行。
- 当前主链路：`MainWindow -> YahooProducer -> HttpClient -> Yahoo API -> DataHub -> panels/charts`。
- SQLite 当前主要缓存股票基础信息和报价；迁移框架只有初始版本。
- DSL、指标引擎、多源聚合器属于可复用模块，但并非全部接入主 UI。

## 诚实的完成度口径

| 能力 | 状态 |
|---|---|
| Qt Dock 工作台 | 已接入 |
| Yahoo 报价/K线 | 已接入 |
| DataHub 订阅与回放 | 已接入 |
| SQLite 基础缓存 | 已接入 |
| 技术指标计算 | 独立模块，部分接入图表 |
| EastMoney/Sina | 独立生产者/聚合原型 |
| 多源 first-valid 竞速 | 未达到生产级 |
| DSL 策略回测 | 未接入 |
| 组合交易 | UI 原型，核心操作未完成 |
| WebSocket / AI Agent | 未实现 |

## 优先风险

1. `HttpClient::get()` 使用嵌套事件循环；主流程调用时可能阻塞 UI。
2. `Aggregator` 在线程池里访问单例网络对象，线程归属需要重构。
3. 指标算法和 Parser 缺少自动化测试；短输入和错误输入必须优先覆盖。
4. CMake 预设包含本机 Qt 路径，跨机器构建前必须改为用户配置或工具链参数。
