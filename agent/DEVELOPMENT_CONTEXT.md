# FinInsight Development Context

> 这是面向后续 Agent 和开发会话的持续交接文档。它记录源码实际状态、最近验证结果和下一步入口，不把路线图或设计想法写成已完成事实。每次完成一项开发后，都要在本文档中沉淀结果。

## 1. 当前目标

FinInsight 正从桌面行情分析原型演进为“模拟投资实验室”：用户使用虚拟资金进行历史或当前行情下的假设交易，获得收益、风险和行为复盘反馈。核心闭环是：

```text
市场数据 -> 模拟决策 -> 持仓/盈亏 -> 风险指标 -> 行为复盘
```

产品是教育和实验工具，不是荐股或自动交易系统。所有 Agent 分析都必须基于交易记录和指标证据，不输出确定性收益承诺或直接下单指令。

## 2. 当前技术与主链路

- 技术栈：C++20、Qt6 Widgets/Charts/Network/Sql/Concurrent、SQLite、CMake/Ninja。
- 运行形态：Windows x64 Qt 桌面应用。
- 当前主链路：`main.cpp -> MainWindow -> YahooProducer -> HttpClient -> Yahoo Finance -> DataHub -> panels/charts`。
- DataHub 是进程内发布/订阅中心；标准数据结构是 `QuoteData` 和 `KLineData`。
- SQLite 当前主要用于股票基础信息、报价和 K 线缓存；迁移框架只有初始版本。

## 3. 真实完成度

| 能力 | 状态 | 说明 |
|---|---|---|
| Qt Dock 工作台 | 已实现 | `MainWindow` 编排自选股、图表、详情和组合面板 |
| Yahoo 报价/K线 | 已接入 | 启动和搜索流程使用 `YahooProducer` |
| DataHub 订阅/回放 | 已实现 | 支持主题匹配和最后值回放 |
| SQLite 基础缓存 | 已实现 | 当前迁移为 V001 |
| K线与部分技术指标 | 部分实现 | `KLineChart` 已显示 MA/BOLL，算法模块更完整 |
| EastMoney/Sina 多源 | 异步适配器/原型 | EastMoney 已使用异步 HTTP；Sina 由共享适配器提供；尚未接入主窗口 |
| Aggregator first-valid | 异步原型已完成 | 按市场路由数据源，支持领域校验、总超时和取消；离线状态机测试已覆盖，尚未接入 MainWindow |
| HTTP 异步请求 | 主链路已接入 | Yahoo、EastMoney 和 Aggregator 已使用异步接口；旧同步接口仅作为兼容实现保留 |
| WebSocket 实时行情 | 实验适配已完成 | Yahoo streamer 订阅和 PricingData Protobuf 最小解析已实现；尚未接入主链路，HTTP 仍为正式回退 |
| Portfolio 模拟交易 | 内存闭环已完成 | 当前标的和 QuoteData 已注入；Ledger 执行买卖、手续费、交易拒绝和估值，面板展示交易记录及盈亏；尚未持久化 |
| 模拟投资账本核心 | 已接入 UI | 纯 C++ 买卖、持仓成本、手续费和盈亏计算已接入 MainWindow/Portfolio；尚未接入 SQLite |
| 历史买入持有实验 | Qt 闭环已完成 | 已接收 MainWindow 日 K 线，支持参数输入、成交边界、期末权益、收益率和最大回撤；尚未持久化 |
| 历史价格转换 | Qt 闭环已完成 | ISO 交易日、标的、顺序及收盘价/复权价口径校验已接入实验面板 |
| DSL 策略回测 | 未接入 | Lexer/Parser/Evaluator 独立存在 |
| AI Agent 行为复盘 | 未实现 | 产品内没有 Agent；`agent/` 目录只是开发文档 |

## 4. 当前优先任务

### P0：稳定基础设施

1. 将 Aggregator 接入 MainWindow 报价主链路，并设计缓存降级和用户可见的数据源状态。
2. 设计 Portfolio/实验输入快照和 SQLite 持久化契约。
3. 为 K 线 JSON 解析、指标和 DSL Parser 补充自动化测试。
4. 将已通过离线测试的 Aggregator 接入 MainWindow 主链路。

### P1：建立业务闭环

1. 为历史实验增加可选基准比较，并保持基准与标的相同的时间和价格口径。
2. 设计实验输入快照，记录数据源、价格口径、日期范围和费用，作为 SQLite 持久化契约。
3. 将交易记录、持仓、实验参数和行情口径持久化到 SQLite，保证结果可复现。
4. 将历史实验接入独立结果界面，UI 只负责输入、状态和结果展示。

### P1/P2：行情能力

1. 在异步请求模型稳定后接入 `Aggregator`，实现 first-valid、超时和取消剩余任务。
2. 评估 Yahoo 实验性 streamer 的协议稳定性和许可边界，再决定是否接入持续行情拉流；当前 WebSocket 尚未进入股票主链路。
3. 明确交易日、时区、复权价格、缺失数据、停牌和公司行动的处理口径。

### P2/P3：分析与工程化

1. 增加波动率、夏普比率、集中度、超额收益等风险归因指标。
2. 将 DSL 绑定历史 K 线，形成可测试的策略回测链路。
3. 先实现确定性行为指标和证据包，再实现产品内 Agent；当前没有运行时 Agent。
4. 增加 Windows CI、CTest、格式检查和故障注入。

## 5. 关键技术决策

- UI 不直接解析供应商 JSON；所有数据源必须转换为 `QuoteData`/`KLineData`。
- Producer 不直接持有面板指针，通过 DataHub 或明确的 Qt signal 输出结果。
- DataHub 回调在锁外执行；订阅者销毁前必须取消订阅。
- SQLite 连接遵守 Qt 线程归属，不在线程间共享同一个 `QSqlDatabase` 实例。
- 模拟交易的成交、估值和风险计算放在可独立测试的领域模块，UI 只负责输入和展示。
- 所有回测结果必须记录输入、价格口径、时间范围、手续费和数据来源。
- `Aggregator` 不能仅以“第一个返回”作为有效结果，必须校验数据完整性、时间戳和标的。

## 6. 已知风险与限制

1. 同步 `HttpClient::get()` 暂时保留兼容接口，尚未从工程中移除。
2. Aggregator 尚未接入 MainWindow 主链路，也未完成缓存降级策略。
3. DataHub 的 Topic 是字符串约定，拼写错误无法在编译期发现。
4. 指标算法、DSL Parser、K 线 JSON 解析和历史实验 Qt 桥接仍缺少完整自动化测试；异步网络、报价适配、Aggregator、模拟核心和 Portfolio Qt 接入已有离线测试。
5. `CMakePresets.json` 含有本机 Qt 路径，跨机器构建前需调整。

## 7. 开发后知识沉淀格式

每完成一个功能或修复，都在“开发记录”新增一条：

```markdown
### YYYY-MM-DD：功能/问题名称
- 修改文件：`path/to/file`
- 实现内容：实际完成的行为和边界
- 验证方式：命令、测试或手工验证结果
- 未解决问题：仍存在的风险或限制
- 下一步：下一会话可直接执行的任务
```

同时更新“真实完成度”和“当前优先任务”，保持状态与源码一致。

## 8. 开发记录

### 2026-08-07：建立持续开发上下文

- 修改文件：新增 `agent/DEVELOPMENT_CONTEXT.md`，更新 `agent/README.md` 索引。
- 实现内容：记录产品目标、主链路、当前完成度、HTTP/Aggregator/WebSocket/模拟账本优先级和后续沉淀规则。
- 验证方式：基于当前源码、项目架构文档和路线图核对模块状态；未执行构建（本机 Qt 环境尚未安装）。
- 未解决问题：异步 HTTP、模拟账本、Aggregator 和 WebSocket 均尚未实现。
- 下一步：优先设计并测试异步 HTTP 请求接口，随后建立模拟投资账本的数据模型。

### 2026-08-07：Yahoo 主链路异步化

- 修改文件：`src/network/HttpClient.h`、`src/network/HttpClient.cpp`、`src/datahub/YahooProducer.cpp`、`src/app/MainWindow.h`、`src/app/MainWindow.cpp`、`src/main.cpp`。
- 实现内容：新增 `HttpResponse`、请求 ID、超时、取消和 HTTP 状态码语义；请求回调通过 queued invocation 返回 context 所在线程。Yahoo 报价与 K 线不再调用同步 `get()`；MainWindow 持有长期存活的 YahooProducer，移除启动时重复 AAPL 请求。
- 验证方式：静态搜索确认 Yahoo 主链路只调用 `getAsync()`；`git diff --check` 通过。`cmake --preset win-dev` 未能配置，因为当前环境缺少 Ninja 和 C++ 编译器，尚未进入 Qt 编译阶段。
- 未解决问题：同步接口仍被 EastMoneyProducer/Aggregator 使用；没有 Qt 环境，尚未执行自动化测试或实际网络验证。
- 下一步：安装完整 Qt/MSVC/Ninja 工具链后，先为 HttpResponse、超时、取消和 context 销毁补 CTest；再迁移 EastMoneyProducer 和重构 Aggregator。

### 2026-08-12：EastMoney 与 Aggregator 异步化

- 修改文件：`src/datahub/EastMoneyProducer.cpp`、`src/datahub/Aggregator.h`、`src/datahub/Aggregator.cpp`。
- 实现内容：EastMoney 使用 `HttpClient::getAsync()`；Aggregator 改为异步状态机，同时请求 Yahoo/EastMoney/Sina，校验 symbol、价格、时间戳和 `QuoteData::isValid()`，首个有效结果发布后取消其余请求；总超时、全部失败和迟到回调只结束一次。
- 验证方式：静态检查确认 `src/datahub` 不再调用同步 `HttpClient::get()`，且已移除 Aggregator 的 `QtConcurrent::run`/`waitForFinished()`；`git diff --check` 通过。当前无 Qt/MSVC/Ninja，未完成编译和运行验证。
- 未解决问题：Aggregator 尚未接入 MainWindow；HTTP 重试和缓存降级未实现；同步 `get()` 仍作为兼容接口保留。
- 下一步：安装工具链后补网络/聚合测试，再将 MainWindow 的行情请求切换到 Aggregator。

### 2026-08-13：共享报价适配器与 Aggregator 路由收敛

- 修改文件：新增 `src/datahub/QuoteAdapters.h`、`src/datahub/QuoteAdapters.cpp`；更新 `src/datahub/Aggregator.*`、`src/datahub/YahooProducer.cpp`、`src/datahub/EastMoneyProducer.*` 和 `src/CMakeLists.txt`。
- 实现内容：抽取 Yahoo/EastMoney/Sina 的 URL 构建和报价解析；Aggregator 按标的路由，六位数字代码只请求 EastMoney/Sina，其余代码请求 Yahoo；完成 first-valid 前先清理状态和取消剩余请求，再调用外部回调，降低可重入导致的状态失效风险。
- 验证方式：静态搜索确认解析逻辑集中到 `QuoteAdapters`，Aggregator 不再直接依赖供应商 JSON；确认同步 `HttpClient::get()` 无生产 Producer 调用；`git diff --check` 通过。当前无 Qt/MSVC/Ninja，未执行编译和运行验证。
- 未解决问题：Aggregator 尚未接入 MainWindow；HTTP 重试、缓存降级和自动化测试尚未实现；同步 GET 仍保留兼容接口。
- 下一步：在 Qt 环境可用后补适配器 fixture、Aggregator 状态机测试，再接入报价主链路和 SQLite 缓存降级。

### 2026-08-13：纯 C++ 行情规则与模拟账本核心

- 修改文件：新增 src/market/QuoteRules.*、src/simulation/Ledger.*、tests/core/*、tests/fixtures/quotes/* 和 docs/SIMULATION.md；更新构建清单与架构文档。
- 实现内容：抽取不依赖 Qt 的标的规范化、数据源路由、报价校验和错误汇总；实现虚拟现金、买卖、长仓限制、加权平均成本、手续费、已实现/未实现盈亏、总权益和收益率；缺少估值价格时明确失败。
- 验证方式：独立 CMake 工程使用 MSVC 19.44 编译成功；ctest --test-dir build/core-tests -C Debug --output-on-failure 通过，1/1 测试成功；git diff --check 通过。
- 未解决问题：fixture 尚未接入 Qt JSON 适配器测试；账本尚未处理交易日、滑点、税费、币种和持久化；PortfolioPanel 尚未调用账本。
- 下一步：继续扩展纯 C++ 账本边界和实验指标；Qt 环境可用后补适配器/Aggregator 测试，并把账本接入 PortfolioPanel 和 SQLite Repository。

### 2026-08-13：历史买入持有实验核心

- 修改文件：新增 `src/simulation/InvestmentExperiment.*`；更新 `tests/core/*`、`src/CMakeLists.txt`、`docs/SIMULATION.md` 和架构文档。
- 实现内容：支持单标的历史区间买入并持有；起点取区间内第一条价格成交，终点取区间内最后一条价格估值；复用账本计算整数股、手续费、剩余现金、期末权益、收益率，并按买入后权益曲线计算最大回撤。乱序、重复时间戳、非法价格、区间无数据、资金不足和数量溢出均返回明确错误。
- 验证方式：独立 CMake 工程使用 MSVC 编译成功；`ctest --test-dir build/core-tests -C Debug --output-on-failure` 通过，1/1 测试成功。
- 未解决问题：尚无历史行情加载器和 Qt/SQLite 接入；暂不处理复权、时区、交易日历、停牌、公司行动、基准比较和期末自动清仓。
- 下一步：优先定义历史 K 线到严格价格序列的转换层及数据口径，再实现基准比较或 SQLite 实验快照持久化。

### 2026-08-13：历史 K 线实验价格转换

- 修改文件：新增 `src/simulation/HistoricalPriceSeries.*` 和 `src/datahub/HistoricalPriceAdapter.*`；更新 `QuoteData.h`、`YahooProducer.cpp`、核心测试、构建清单及相关文档。
- 实现内容：纯 C++ 转换层校验统一标的、严格 ISO 交易日、唯一递增顺序和有限正价格；调用方显式选择普通收盘价或复权收盘价，复权数据缺失时明确失败。交易日以 UTC 零点作为稳定排序键。Qt 桥接将 `QVector<KLineData>` 映射为领域输入，Yahoo 解析器使用 UTC 日期并保留可选 `adjclose`。
- 验证方式：独立 CMake 工程使用 MSVC 编译成功；`ctest --test-dir build/core-tests -C Debug --output-on-failure` 通过，1/1 测试成功；覆盖闰年日期、顺序、重复日期、标的一致性、两种价格口径和转换后运行实验。
- 未解决问题：当前环境没有 Qt，桥接层和 Yahoo 解析改动仅完成静态检查；未处理盘中时间、交易所时区、公司行动语义和跨数据源复权一致性。
- 下一步：实现基准序列对齐和基准收益/超额收益，或先定义 SQLite 实验输入快照持久化模型。

### 2026-08-14：项目状态审计与后续规划整理

- 修改文件：更新 `agent/README.md`、`agent/PROJECT_CONTEXT.md`、`agent/ROADMAP.md`、`agent/ARCHITECTURE.md` 和本文档。
- 实现内容：核对源码后明确区分了“核心已实现”“UI 原型”“异步原型”“尚未接入”和“完全未实现”。确认 PortfolioPanel 已创建但尚未调用 Ledger；Aggregator 尚未替换 MainWindow 主链路；WebSocket 没有客户端或协议接入；产品内 AI Agent 尚未实现，`agent/` 仅为开发文档。
- 验证方式：静态搜索 `PortfolioPanel`、`Ledger`、`Aggregator`、`QWebSocket`、Agent/LLM 相关符号和 CMake 链接项；与独立核心 CTest 结果交叉核对。
- 未解决问题：Qt 主工程仍未在当前环境完成编译；模拟 UI、SQLite 实验持久化、基准比较、WebSocket、Agent 和 DSL 回测仍需后续实现。
- 下一步：优先在 Qt 环境可用后完成 `PortfolioPanel -> Ledger` 接入，再接历史实验 UI；随后处理 Aggregator 主链路和 SQLite 快照。

### 2026-08-16：Qt 主工程编译与启动验证

- 修改文件：`src/datahub/YahooProducer.cpp`、`build.bat`、`agent/README.md`、`agent/PROJECT_CONTEXT.md`、`agent/ROADMAP.md` 和本文档。
- 实现内容：为 Yahoo K 线 JSON 解析补齐 `QJsonArray`、`QJsonDocument` 和 `QJsonObject` 显式头文件，使近期历史价格桥接改动能够在完整 Qt 工程中编译；修正构建脚本输出的可执行文件路径。
- 验证方式：使用 Qt 6.7.3、MSVC 19.44 和 Ninja 完成 `cmake --preset win-dev` 及完整构建；`windeployqt` 成功；独立核心 CTest 通过，1/1 测试成功；启动 `FinInsight.exe` 后主窗口标题、窗口句柄和响应状态正常，持续运行 6 秒后主动结束测试进程。
- 未解决问题：Qt 网络、JSON 适配器和 Aggregator 仍没有自动化测试；启动冒烟测试未验证外部行情服务的返回质量；PortfolioPanel 和历史实验 UI 仍未接入领域核心。
- 下一步：补 Qt 异步网络和 Aggregator 离线测试，然后完成 `PortfolioPanel -> Ledger` 接入。

### 2026-08-16：异步网络、报价适配和 Aggregator 离线测试

- 修改文件：`CMakeLists.txt`、`tests/qt/*`、`src/network/HttpClient.cpp`、`src/datahub/Aggregator.*` 及相关 agent 文档。
- 实现内容：新增 Qt Test/CTest 目标；通过回环 TCP 服务覆盖异步 HTTP 成功、HTTP 错误、超时、主动取消和 context 销毁；使用既有 fixture 覆盖 Yahoo/EastMoney/Sina 报价解析；为 Aggregator 增加保持默认行为的传输注入边界，覆盖 first-valid 取消慢请求、无效载荷回退、全部失败、总超时和迟到回调。超时/取消后不再读取已关闭的 QNetworkReply。
- 验证方式：完整 Qt 工程构建成功；Qt Test 10/10 通过，CTest 连续运行 10 轮无失败；独立核心 CTest 1/1 通过；所有测试均离线运行。
- 未解决问题：尚未测试真实供应商服务和网络代理差异；K 线 JSON 解析、指标、DSL Parser 及历史实验 Qt 桥接仍缺少自动化测试；Aggregator 尚未接入 MainWindow。
- 下一步：在已有离线状态机保障下接入 Aggregator 主链路，或优先完成 `PortfolioPanel -> Ledger` 业务闭环。

### 2026-08-16：Portfolio 模拟交易内存闭环

- 修改文件：`src/panels/PortfolioPanel.*`、`src/app/MainWindow.cpp`、`tests/qt/*` 及相关项目文档。
- 实现内容：移除 PortfolioPanel 自有的现金、持仓和交易计算，改由唯一 `simulation::Ledger` 执行；MainWindow 注入当前标的和最新 QuoteData；支持数量、手续费、买卖、余额/持仓不足提示、现金、持仓市值、总权益、已实现/未实现盈亏、收益率和账本成交记录。Dock 明确标记为 Simulation Portfolio。
- 验证方式：新增 QWidget 离线测试，覆盖无报价禁用、余额不足、买入、报价重估、部分卖出、手续费、实现/未实现盈亏和持仓不足；完整 Qt CTest 2/2、核心 CTest 1/1 通过；主窗口启动并响应正常；截图核对默认 Dock 高度下摘要、交易控件和成交表均可见。
- 未解决问题：账户仍为内存状态，重启后重置；没有 SQLite Repository、撤单、滑点、税费、交易日历或多币种处理；报价没有陈旧状态提示。
- 下一步：接入历史投资实验 UI，或将已通过离线测试的 Aggregator 接入 MainWindow 主链路。

### 2026-08-16：历史实验 Qt UI 闭环

- 修改文件：`src/panels/ExperimentPanel.*`、`src/app/MainWindow.*`、`tests/qt/experiment_panel_tests.cpp`、`src/CMakeLists.txt` 及相关文档。
- 实现内容：新增 Historical Experiment 底部标签；接收当前标的日 K 线，支持起止日期、初始资金、买入手续费、Close/Adjusted Close 口径，并展示成交日期/价格、期末现金、市值、权益、收益率和最大回撤。数据流固定为 `HistoricalPriceAdapter -> HistoricalPriceSeries -> InvestmentExperiment`。
- 验证方式：完整 Qt 构建通过；Qt CTest 3/3、核心 CTest 1/1 通过；启动 AAPL 实际运行并完成面板截图检查。
- 未解决问题：实验结果和配置仍只在内存中；暂不处理交易日历、时区、停牌、公司行动、滑点、税费和多币种；Aggregator 仍未替换 MainWindow 主链路，WebSocket 仍未实现。
- 下一步：优先设计实验/组合 SQLite 快照契约，或在明确数据源和协议后实现 WebSocket 客户端；两者都必须保持现有领域核心和离线测试可复现。

### 2026-08-16：WebSocket 传输层与回环测试

- 修改文件：`CMakeLists.txt`、`src/CMakeLists.txt`、`src/network/WebSocketClient.*`、`tests/qt/CMakeLists.txt`、`tests/qt/websocket_client_tests.cpp` 及相关文档。
- 实现内容：启用 Qt WebSockets；新增供应商无关的 `WebSocketClient`，支持连接/主动断开、原始文本和二进制消息、可配置 ping 心跳、指数退避重连和重连次数限制。客户端不解析供应商 JSON，不直接发布 DataHub 主题。
- 验证方式：完整 Qt 构建成功；Qt CTest `4/4` 通过，包含本地 `QWebSocketServer` 的连接、echo、断开和禁止重连场景；`git diff --check` 通过。
- 未解决问题：Yahoo streamer 是非官方实验接口，消息协议和许可边界可能变化；尚未接入 Aggregator/MainWindow 股票主链路，仍需保留 HTTP 回退。
- 下一步：在保持适配器独立的前提下，增加真实消息 fixture 和连接状态 UI；不要直接替换 Yahoo HTTP 主链路。

### 2026-08-16：Yahoo 实验性股票 streamer 适配

- 修改文件：`src/datahub/YahooWebSocketAdapter.*`、`src/datahub/DataHub.*`、`tests/qt/websocket_client_tests.cpp` 及相关构建和文档。
- 实现内容：使用无需 API Key 的 Yahoo streamer 实验接口；发送 `subscribe/unsubscribe` JSON，解析 `PricingData` Protobuf 二进制字段为 `QuoteData`，发布 `<symbol>.quote.realtime`，并在重连后恢复订阅。
- 验证方式：本地 WebSocket fixture 模拟订阅和 PricingData 二进制消息；Qt CTest `4/4` 通过，完整 Qt 构建通过。
- 未解决问题：该接口非官方，可能变更、限流或不适用于生产；尚未实现协议版本探测、真实联网验证、Aggregator/MainWindow 接入和 HTTP 回退编排。

### 2026-08-16：Agent 确定性行为分析基础

- 修改文件：`src/analysis/BehaviorAnalyzer.*`、`tests/core/core_tests.cpp`、`tests/core/CMakeLists.txt` 及相关文档。
- 实现内容：定义 `EvidenceSnapshot` 和 `BehaviorReport`，从 Ledger 交易证据计算交易频率、买入集中度、卖出胜率、盈亏因子、平均盈亏及回撤期间交易，并生成带代码、严重级别和度量值的结构化 findings。分析器不调用 LLM、不修改账本，也不对缺失证据做推断。
- 验证方式：核心 CTest 通过；Qt 主工程构建和 Qt CTest `4/4` 通过；`git diff --check` 通过。
- 未解决问题：Portfolio/实验结果尚未持久化到 SQLite；尚无证据包版本化、Agent 面板和 LLM 接口。
- 下一步：设计 SQLite evidence snapshot migration，并为 findings 增加时间线和来源引用，再评估是否接入本地或远程模型。

### 2026-08-16：Agent 预备证据持久化接入

- 修改文件：`src/storage/migrations/V002_Evidence.h`、`src/storage/EvidenceSnapshotRepository.*`、`src/panels/PortfolioPanel.*`、`src/panels/ExperimentPanel.*`、`src/app/MainWindow.*` 及 Qt Repository 测试。
- 实现内容：新增 evidence snapshot、交易明细和 findings 三表；保存 schema version、source、price basis、时间范围、估值和回撤区间。Portfolio 成交成功后、历史实验完成后由 MainWindow 自动保存快照，面板不直接依赖 SQLite。
- 验证方式：Qt CTest `5/5`、核心 CTest `1/1` 通过；完整 Qt 构建和 `git diff --check` 通过。
- 未解决问题：尚无快照列表/详情 UI，现有 Repository 只有保存和计数接口；Portfolio 的回撤曲线和历史实验的完整价格序列仍未持久化。
- 下一步：增加快照查询/删除策略和时间线模型，再设计 Agent 面板的证据引用展示。

### 2026-08-16：Agent EvidenceBundle 查询接口

- 修改文件：`src/storage/EvidenceSnapshotRepository.*` 和 `tests/qt/evidence_repository_tests.cpp`。
- 实现内容：新增统一 `Detail` 查询模型，通过 `load(snapshotId)` 一次读取快照摘要、按时间排序的交易证据和结构化 findings；与 `recent()`、`remove()` 共同形成 Agent 面板所需的完整 Repository 边界。
- 验证方式：完整 Qt 构建通过；Qt CTest `5/5`、核心 CTest `1/1` 通过；`git diff --check` 通过。
- 未解决问题：尚未实现快照历史面板和 finding 到交易 ID 的显式关联；LLM 仍未接入。
- 下一步：实现只读 Agent Review 面板，先展示确定性报告和证据明细，再评估模型接口。

### 2026-08-16：Agent Review 只读面板

- 修改文件：`src/panels/AgentReviewPanel.*`、`src/app/MainWindow.*` 及构建清单。
- 实现内容：新增底部 Agent Review 标签，读取 Repository 最近快照；选择快照后展示来源/价格口径、权益/收益/回撤摘要、结构化 findings 和交易时间线。Portfolio 成交或历史实验完成保存后自动刷新面板。面板不调用 LLM，也不修改账本。
- 验证方式：完整 Qt 构建通过；Qt CTest `5/5`、核心 CTest `1/1` 通过；`git diff --check` 通过。
- 未解决问题：finding 尚未关联具体 trade id；面板暂无过滤、分页和删除操作；LLM 生成层尚未实现。
- 下一步：为 findings 增加证据引用字段和时间线高亮，再定义 `ReviewGenerator` 抽象及离线 deterministic 实现。

### 2026-08-16：Agent 证据引用与确定性复盘生成器

- 修改文件：`src/analysis/BehaviorAnalyzer.*`、`src/analysis/ReviewGenerator.*`、`src/storage/migrations/V003_FindingEvidence.h`、`src/storage/EvidenceSnapshotRepository.*` 及测试。
- 实现内容：findings 保存证据时间范围和交易 ID 列表；新增 V003 migration 和详情读取；新增 `DeterministicReviewGenerator`，把报告转换为可复现的事实段落并保留 finding code/交易 ID 引用。
- 验证方式：完整 Qt 构建通过；Qt CTest `5/5`、核心 CTest `1/1` 通过；`git diff --check` 通过。
- 未解决问题：当前 findings 默认关联快照内全部交易，尚未细化到规则相关交易子集；LLM、模型超时/脱敏和自然语言面板仍未实现。
- 下一步：细化每条规则的交易证据集合，并把 deterministic review 接入 Agent Review 面板。

### 2026-08-16：Deterministic Review 面板接入

- 修改文件：`src/panels/AgentReviewPanel.*` 及相关构建和文档。
- 实现内容：Review 面板直接调用 `DeterministicReviewGenerator` 展示复盘段落；点击 finding 会按保存的交易 ID 高亮证据交易行。面板继续保持只读和离线行为。
- 验证方式：完整 Qt 构建通过；Qt CTest `5/5`、核心 CTest `1/1` 通过；`git diff --check` 通过。
- 未解决问题：当前规则仍把快照内交易作为默认证据集合；尚未增加 finding 过滤、分页和 LLM ReviewGenerator 实现。
- 下一步：增加规则级交易筛选和模型接口隔离层，保持 deterministic review 在模型不可用时可独立工作。

## 9. 下一会话入口

开始新会话时按以下顺序读取：

1. 本文件 `DEVELOPMENT_CONTEXT.md`。
2. `PROJECT_CONTEXT.md` 和 `ARCHITECTURE.md`。
3. 与任务相关的 `docs/PROJECT_ARCHITECTURE.md` 和模块文档。
4. 搜索当前调用方、DataHub Topic 和测试入口后再修改代码。

结束会话前必须更新本文件的真实完成度、开发记录、验证方式和下一步入口。
