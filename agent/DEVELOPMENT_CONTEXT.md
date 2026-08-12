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
| EastMoney/Sina 多源 | 异步适配器/原型 | EastMoney 已使用异步 HTTP；尚未接入主窗口 |
| Aggregator first-valid | 异步原型已完成 | 已支持并发请求、领域校验、总超时和取消；尚未编译/运行验证 |
| HTTP 异步请求 | 主链路已接入 | Yahoo、EastMoney 和 Aggregator 已使用异步接口；旧同步接口仅作为兼容实现保留 |
| WebSocket 实时行情 | 未实现 | 需先确认可用数据源和协议 |
| Portfolio 模拟交易 | UI 原型 | 买卖、持仓、报价联动和持久化未完成 |
| DSL 策略回测 | 未接入 | Lexer/Parser/Evaluator 独立存在 |
| AI Agent 行为复盘 | 未实现 | 等账本和风险指标可信后再做 |

## 4. 当前优先任务

### P0：稳定基础设施

1. 为异步 HTTP 和 Aggregator 补充重试策略、离线 fixture、错误路径和生命周期测试。
2. 将 Aggregator 接入 MainWindow/DataHub 主链路，并移除不再使用的同步 HTTP 调用。
3. 明确缓存降级和多源数据质量评分规则。

### P1：建立业务闭环

1. 新增独立模拟投资账本领域模块，不把交易计算继续堆进 `PortfolioPanel`。
2. 支持虚拟现金、模拟成交、持仓成本、已实现/未实现盈亏、手续费和交易日规则。
3. 将交易记录、持仓、实验参数和行情口径持久化到 SQLite，保证结果可复现。
4. 支持历史时点假设交易，并计算收益率、最大回撤和基准对比。

### P1/P2：行情能力

1. 在异步请求模型稳定后接入 `Aggregator`，实现 first-valid、超时和取消剩余任务。
2. 在确认稳定 WebSocket 数据源、消息协议和重连策略后，再增加持续行情拉流。
3. 明确交易日、时区、复权价格、缺失数据、停牌和公司行动的处理口径。

### P2/P3：分析与工程化

1. 增加波动率、夏普比率、集中度、超额收益等风险归因指标。
2. 将 DSL 绑定历史 K 线，形成可测试的策略回测链路。
3. 基于交易证据实现 Agent 行为复盘，并保留模拟免责声明。
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
4. 指标算法、Parser、JSON 解析、异步网络和模拟交易尚缺少完整自动化测试。
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

## 9. 下一会话入口

开始新会话时按以下顺序读取：

1. 本文件 `DEVELOPMENT_CONTEXT.md`。
2. `PROJECT_CONTEXT.md` 和 `ARCHITECTURE.md`。
3. 与任务相关的 `docs/PROJECT_ARCHITECTURE.md` 和模块文档。
4. 搜索当前调用方、DataHub Topic 和测试入口后再修改代码。

结束会话前必须更新本文件的真实完成度、开发记录、验证方式和下一步入口。
