# WebSocket、Agent 与实时交易一日开发规划

更新日期：2026-08-17

## 1. 当日目标

在现有 FinInsight 基础上完成三个可运行、可测试、边界明确的大模块：

1. 将 WebSocket 实时行情从独立实验适配器接入统一行情链路。
2. 将确定性复盘升级为可选模型生成的 Agent，同时保留离线回退。
3. 建立实时交易领域层、持久化、风控、模拟执行及一个官方券商适配器。

“完成”是指代码、UI、持久化、错误状态和自动化测试形成闭环。真实账户最终能否成交仍取决于用户提供的合法券商账户、API 权限和凭据；没有凭据时必须能通过本地模拟服务和券商 paper 环境验证相同协议。

## 2. 已有基础

- `network::WebSocketClient` 已支持连接、心跳、主动断开和退避重连。
- `YahooWebSocketAdapter` 已能解析实验性 Yahoo PricingData 并发布实时主题。
- `DataHub`、HTTP 行情和 Aggregator 已存在，HTTP 可作为实时流故障时的回退。
- `simulation::Ledger` 已承载当前模拟成交与估值。
- V002/V003 已保存 Agent evidence、交易和 finding 引用。
- `BehaviorAnalyzer`、`DeterministicReviewGenerator` 和只读 `AgentReviewPanel` 已形成离线复盘链路。
- Qt CTest 与核心 CTest 已覆盖网络、WebSocket、组合、实验、Repository 和 Agent 面板。

## 3. 总体架构

```text
HTTP sources --------------------------+
                                       v
WebSocket feed -> QuoteStreamService -> Quote quality/freshness -> DataHub -> UI
                                                        |
                                                        +------> Paper matcher

Order UI -> OrderService -> RiskEngine -> ExecutionGateway
                                      +-> PaperExecutionGateway
                                      +-> AlpacaExecutionGateway -> official broker API
                                                   |
                                                   v
                              OrderRepository <- broker order/account stream
                                      |
                                      v
                         evidence snapshot -> Agent review

EvidenceRepository -> BehaviorAnalyzer -> ReviewGenerator
                                      +-> DeterministicReviewGenerator
                                      +-> OpenAICompatibleReviewGenerator
```

强制边界：

- WebSocket 适配器只负责连接协议和消息规范化，不直接操作界面或账本。
- 券商账户、订单和成交是实盘状态的权威来源，本地 SQLite 只保存镜像与审计记录。
- Agent 只读取已保存 evidence，不持有 `OrderService`、`ExecutionGateway` 或券商凭据。
- 模型失败、无配置或超时时继续显示确定性复盘，不能阻塞 UI。
- 实盘模式默认关闭，必须显式配置账户、解除开关并逐单确认。

## 4. 模块 A：WebSocket 正式接入

### 4.1 领域接口

- 新增 `IQuoteStream`，统一 `start/stop/subscribe/unsubscribe`、连接状态和规范化报价信号。
- 让 `YahooWebSocketAdapter` 实现该接口，继续标记为 Experimental。
- 新增 `QuoteStreamService` 管理订阅引用计数、重连后的恢复订阅、最后消息时间和源状态。
- 实时消息统一转成 `QuoteData`，进入 DataHub 的 `<symbol>.quote.realtime`。

### 4.2 主链路编排

- MainWindow 只订阅 DataHub，不直接持有供应商消息解析逻辑。
- 实时报价通过现有报价规则校验有限值、正价格、标的匹配和时间戳。
- 增加 `Live/Fallback/Stale/Disconnected/Experimental` 状态。
- 流断开或报价超过 freshness 阈值时触发 HTTP 刷新；WebSocket 恢复后自动回到实时状态。
- 当前 Yahoo HTTP 主链路继续保留，不因实验流不可用而影响基本行情。

### 4.3 UI

- 在状态栏或行情详情中显示来源、连接状态和最后更新时间。
- 提供实验实时行情开关和连接/停止控制。
- 切换标的时正确取消旧订阅、订阅新标的，关闭窗口时释放连接。

### 4.4 测试与验收

- 本地 `QWebSocketServer` 覆盖连接、订阅、消息、断线、重连和恢复订阅。
- 覆盖坏消息、错误 symbol、无效价格、乱序/陈旧消息。
- 覆盖 WebSocket 断开后 HTTP fallback，以及恢复后状态切回。
- 验收：AAPL 等标的能在 UI 中持续刷新；断开本地流时 UI 不冻结，状态明确且 HTTP 仍可用。

## 5. 模块 B：Agent 正式接入

### 5.1 生成接口

- 抽象 `ReviewGenerator` 接口，输入固定为版本化 `EvidenceBundle`，输出为结构化 `ReviewResult`。
- `DeterministicReviewGenerator` 实现该接口并作为始终可用的默认实现。
- 新增 `OpenAICompatibleReviewGenerator`，支持配置 base URL、model、API key、timeout 和 max tokens。
- 网络调用异步执行，支持取消；响应必须带 request ID，防止旧请求覆盖当前快照。

### 5.2 安全与输出契约

- 发送前仅组装必要的快照摘要、finding、指标和关联交易，不发送数据库路径或无关账户信息。
- 输出结构区分 `facts`、`inferences`、`educational_notes`、`risks` 和 `evidence_refs`。
- 每条事实或推断必须引用 finding code 或 trade ID；无法解析结构化输出时回退确定性复盘。
- 系统提示明确禁止收益承诺、荐股、直接买卖指令和自动下单。
- Agent 模块不注册任何交易工具，也不暴露实时交易执行对象。

### 5.3 配置、持久化和 UI

- API key 优先从环境变量读取，不写入日志、evidence 或普通配置表。
- 保存非敏感配置：provider、base URL、model、timeout、token limit 和启用状态。
- Agent Review 增加生成、取消、重试、离线回退状态以及模型/确定性来源标识。
- 保存生成结果元数据：快照 ID、provider、model、时间、耗时、状态和证据引用；不保存 API key。

### 5.4 测试与验收

- 本地 HTTP fake 覆盖成功、超时、取消、401、429、5xx、非法 JSON 和引用缺失。
- 验证无 API key 时不发起网络调用，并正常展示 deterministic review。
- 验证切换快照后迟到响应不会污染新快照。
- 验收：配置兼容接口后能生成结构化复盘；断网和模型错误时 UI 有明确状态且离线复盘仍可用。

## 6. 模块 C：实时交易实现

### 6.1 统一交易模型

- 新增 `trading` 模块，定义 `OrderRequest`、`Order`、`Fill`、`AccountSnapshot`、`Position` 和错误类型。
- 订单状态至少包含 `Created`、`RiskRejected`、`Submitting`、`Accepted`、`PartiallyFilled`、`Filled`、`CancelPending`、`Cancelled`、`Rejected`、`Expired` 和 `Unknown`。
- 使用唯一 client order ID 保证提交幂等；券商 order ID 作为外部关联键。
- 模拟账本和实盘账户模型分离，实盘状态不得以 `simulation::Ledger` 为权威。

### 6.2 服务边界

- `ExecutionGateway`：查询账户/持仓/订单、提交订单、撤单、订阅订单和账户事件。
- `OrderService`：订单生命周期、幂等、状态迁移、持久化和 UI 信号。
- `RiskEngine`：下单前校验，不允许 UI 绕过。
- `ReconciliationService`：启动、重连和超时状态未知时从券商重新同步。
- `OrderRepository`：保存订单、状态事件、成交、账户快照和审计记录。

### 6.3 执行适配器

- `PaperExecutionGateway` 使用规范化实时行情撮合，支持市价单、限价单、部分成交模拟、撤单和手续费。
- 选择 Alpaca 作为首个官方券商适配器：其 paper/live API 边界清晰，股票交易和账户流都有正式接口。
- `AlpacaExecutionGateway` 实现账户、持仓、订单查询，下单、撤单和订单更新流。
- paper 与 live endpoint 使用同一适配器配置切换，但 live 必须经过独立启用开关和逐单确认。
- 无凭据时通过本地 fake broker 验证完整协议，不伪造真实成交结论。

### 6.4 基础风控

- 单笔最大金额和最大数量。
- 单标的最大持仓比例。
- 每日最大订单数和最大已实现亏损。
- 报价陈旧、价格偏离、无可用报价和市场状态检查。
- 重复 client order ID 拒绝。
- 全局 kill switch、禁止新单、撤销全部活动订单。
- 实盘下单确认对话框明确显示账户、环境、标的、方向、数量、类型和预计金额。

### 6.5 UI

- 新增实时交易面板：环境、账户状态、资金、持仓、订单输入、活动订单、成交和事件日志。
- 环境必须醒目标识 `PAPER` 或 `LIVE`，默认始终为 `PAPER`。
- 下单按钮根据连接、行情 freshness、风控和 kill switch 状态启用。
- 支持撤单、刷新账户、重新对账和停止交易。

### 6.6 测试与验收

- 状态机测试覆盖接受、拒绝、部分成交、全部成交、撤单竞态、超时未知和重连恢复。
- 风控测试覆盖每个限制及边界值。
- Repository 测试验证订单事件按顺序持久化及重启重建。
- 本地 fake broker 覆盖重复请求、迟到响应、断流和账户重新同步。
- Paper gateway 端到端测试覆盖实时 tick -> 撮合 -> fill -> position/account -> evidence。
- 验收：无需真实凭据即可完成实时 paper 交易闭环；配置合法 Alpaca paper 凭据后可查询账户并完成官方 paper 下单/撤单；live 只在显式解锁后开放。

## 7. 配套能力

以下五项已确认纳入当日开发范围。它们复用三大模块的领域事件和状态，不建立第二套行情、交易或 Agent 逻辑。

### 7.1 统一通知中心

- 新增 `NotificationService`，统一消息类型、严重级别、已读状态、时间戳和来源模块。
- 新增 `TradingNotificationBridge`，监听订单提交、风控拒绝、部分/全部成交、撤单失败和对账异常。
- 接入 WebSocket 断线/恢复、HTTP fallback、Agent 超时/回退等系统事件。
- Qt 界面提供通知入口、未读计数、历史列表、按模块过滤和全部已读。
- 首版包含应用内通知与桌面通知；外部渠道使用 provider 接口预留，不把凭据写入普通日志。
- 测试去重、严重级别、已读持久化、桥接事件和通知风暴限频。

### 7.2 行情与交易健康监控

- 新增 `HealthMonitor`，采集 WebSocket 状态、最后 tick 时间、重连次数、坏消息数和 HTTP fallback 次数。
- 新增 `LatencyTracker`，记录风控耗时、券商 RTT、订单总耗时和 Agent 请求耗时。
- 提供固定容量的内存 ring buffer，并将关键异常摘要持久化；不无限增长日志。
- UI 展示连接健康、P50/P95/P99 延迟、失败率、数据 freshness 和最近异常。
- 状态只用于诊断和保护：陈旧行情可阻止新单，但监控模块本身不能发单。
- 使用可控时钟测试阈值、统计值、ring 容量和健康状态转换。

### 7.3 标的与券商代码解析

- 新增 `Instrument`、`InstrumentRepository`、`InstrumentSource` 和 `SymbolResolver`。
- 统一规范化 symbol、市场、交易所、币种、资产类型和券商 instrument ID/token。
- WebSocket、HTTP、Paper gateway 和 Alpaca gateway 均通过 resolver 获取外部标识。
- 对无法唯一匹配、市场不一致和过期映射明确失败，不使用静默猜测。
- 首版支持当前股票标的及 Alpaca 映射，并提供离线 fixture 导入和刷新入口。
- 测试规范化、别名、冲突、缺失、缓存刷新和跨来源映射。

### 7.4 崩溃恢复与工作区快照

- 新增 `WorkspaceSnapshotRing`，周期保存当前标的、面板可见性、布局、实验输入和非敏感筛选条件。
- 新增 `CrashRecovery`，使用 clean-shutdown marker 判断异常退出，并在启动时提供恢复选择。
- 使用原子写入、schema version、校验和与固定数量快照，损坏快照自动回退到上一份。
- 不保存 API key、access token 或实盘解锁状态。
- 活动订单、资金和持仓不能从工作区快照恢复；启动后必须由 `ReconciliationService` 从券商同步。
- 测试正常关闭、异常标记、损坏文件、版本不兼容、快照轮转和敏感字段排除。

### 7.5 快捷命令面板

- 新增 `CommandPalette` 和命令注册表，支持键盘搜索、分类、最近命令和参数提示。
- 首批命令：切换标的、打开实时行情/交易/Agent/健康面板、刷新行情、重新对账、切换 Paper 环境和打开通知。
- 高风险命令不得绕过现有保护：实盘解锁、提交订单、kill switch 和撤销全部订单仍使用原服务的确认/风控流程。
- Agent 不获得命令面板执行权；命令来源保持为用户交互。
- 测试命令发现、模糊搜索、禁用状态、参数解析、确认委托和快捷键冲突。

## 8. 当日执行顺序

### 阶段 0：基线与共享契约

- 运行现有 Qt/Core 测试，记录基线。
- 固定 `QuoteData`、EvidenceBundle 和数据库迁移兼容边界。
- 为新模块建立独立 CMake target 和测试入口。

### 阶段 1：WebSocket 主链路

- 完成 `IQuoteStream`、`QuoteStreamService`、DataHub 编排、状态 UI 和 fallback 测试。
- 先稳定实时数据输入，供 paper matcher 和交易风险检查复用。

### 阶段 2：实时交易领域闭环

- 完成类型、状态机、Repository、RiskEngine、Paper gateway 和交易面板。
- 完成 Alpaca gateway 及本地 fake broker；再接账户/订单流和对账。

### 阶段 3：Agent 模型接入

- 完成 `ReviewGenerator` 抽象、OpenAI-compatible provider、策略限制和 UI 状态。
- 将交易订单/成交证据纳入复盘输入，但保持严格只读。

### 阶段 4：配套能力

- 先完成 SymbolResolver，统一 WebSocket、Paper 和 Alpaca 的标的映射。
- 接入 NotificationService、HealthMonitor 和 LatencyTracker，覆盖三大模块事件。
- 完成 WorkspaceSnapshotRing、CrashRecovery 和 CommandPalette。

### 阶段 5：集成与回归

- 跑完整 Qt/Core CTest。
- 启动主窗口进行 WebSocket、paper order、Agent fallback 三条烟测。
- 检查数据库迁移、无凭据启动、断网降级、关闭窗口资源释放。
- 更新 `CURRENT_STATUS.md`、`DEVELOPMENT_CONTEXT.md`、架构和用户文档。

## 9. 分批提交颗粒度

1. `feat: integrate realtime quote stream service`
2. `test: cover websocket fallback and freshness states`
3. `feat: add trading domain and order persistence`
4. `feat: add paper execution and pre-trade risk controls`
5. `feat: add alpaca execution gateway and reconciliation`
6. `feat: add realtime trading panel`
7. `feat: add review generator interface and model provider`
8. `test: cover agent failures and evidence references`
9. `feat: add instrument repository and symbol resolver`
10. `feat: add notifications and runtime health monitoring`
11. `feat: add workspace crash recovery`
12. `feat: add command palette`
13. `docs: update realtime trading and agent status`

每个提交只包含对应模块及必要测试，不混入无关文档或格式化。

## 10. 完成定义

- 三个模块均可从 UI 进入并显示明确运行状态。
- 无网络、无模型 key、无券商 key 时应用仍能启动并使用 HTTP 行情、确定性复盘和本地 paper 交易。
- 所有网络路径都有超时、取消、错误展示和资源释放。
- 所有订单状态变化和 Agent 结果都可追溯到本地证据。
- Agent 无法触发交易；交易必须通过 RiskEngine 和 ExecutionGateway。
- 通知与健康面板能追踪实时行情、订单和 Agent 的关键异常，且具备限频。
- WebSocket 和券商适配器共享统一标的解析结果，歧义映射不会进入交易链路。
- 异常退出后可恢复非敏感工作区；券商账户状态仍通过重新对账恢复。
- 快捷命令不能绕过实盘确认、风险检查或 kill switch。
- Qt/Core 全量测试通过，构建成功，`git diff --check` 无实际错误。

## 11. 明确不包含

以下内容不在本文件的三大模块承诺内，只有用户确认后才追加：

- 多券商批量接入。
- 无人值守策略自动下单。
- 期权、多腿、保证金和复杂订单算法。
- 外部社交/消息渠道的完整 provider 集合；首版只完成应用内、桌面通知和扩展接口。
- 工作区跨设备云同步。
