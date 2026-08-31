# Agent 开发入口

本目录是 FinInsight 的持续开发知识库，给后续 agent 或新加入的开发者使用。这里记录“代码当前是什么状态”和“修改时应遵守什么边界”，不把规划写成已完成事实。

## 文件索引

- [`PROJECT_CONTEXT.md`](PROJECT_CONTEXT.md)：项目定位、真实完成度、关键风险和面试口径。
- [`ARCHITECTURE.md`](ARCHITECTURE.md)：模块边界、依赖方向、数据流和线程模型。
- [`DEVELOPMENT_RULES.md`](DEVELOPMENT_RULES.md)：提交前检查、代码风格、测试和文档要求。
- [`ROADMAP.md`](ROADMAP.md)：按优先级排列的后续迭代路线。
- [`DEVELOPMENT_CONTEXT.md`](DEVELOPMENT_CONTEXT.md)：面向后续会话的实时开发上下文、开发记录和下一步入口。
- [`CURRENT_STATUS.md`](CURRENT_STATUS.md)：本轮已实现能力、验证结果和下一阶段方向的简明快照。

## 当前状态速览（2026-08-16）

已真实实现并通过独立核心测试的部分：

- 异步 HTTP 请求模型：响应、超时、取消和生命周期语义；Yahoo/EastMoney/Aggregator 已使用异步接口。
- 行情规则与适配：标的规范化、A 股路由、报价校验、Yahoo/EastMoney/Sina 适配器。
- 模拟核心：纯 C++ 账本、单标的历史买入持有实验、历史 K 线到严格价格序列的转换。
- Qt 行情工作台：搜索、自选股、报价详情、K 线和 MA/BOLL 展示；Portfolio 已接入内存账本和实时报价估值。
- Qt 主工程已使用 Qt 6.7.3/MSVC 19.44 完成编译、部署和窗口启动冒烟验证。
- Qt 离线测试已覆盖报价 fixture 解析、异步 HTTP 成功/错误/超时/取消/context 销毁，以及 Aggregator first-valid、回退、全失败、总超时和迟到回调。
- Agent 基础已增加纯 C++ `BehaviorAnalyzer`，从账本证据输出可测试的频率、集中度、盈亏不对称和回撤期交易 findings；尚未接入 LLM 或 SQLite。

尚未实现或尚未接入主链路的部分：

- Portfolio 交易和估值尚未接入应用主流程的 SQLite 持久化；Agent evidence snapshot Repository 已具备独立保存能力。
- 历史实验已接入 Qt UI 和 MainWindow 的 K 线加载流程；结果仍未持久化到 SQLite。
- Aggregator 尚未替换 MainWindow 当前的 YahooProducer 请求。
- WebSocket 传输层和实验性 Yahoo streamer 适配已实现并有本地回环测试；尚未接入 MainWindow 股票主链路。
- 产品内 AI Agent 行为复盘完全未实现；本目录中的“agent”是开发文档，不是运行时 Agent。
- 基准比较、波动率、夏普比率、组合再平衡、DSL 回测尚未完成。

验证命令：

```powershell
cmake -S tests/core -B build/core-tests
cmake --build build/core-tests --config Debug
ctest --test-dir build/core-tests -C Debug --output-on-failure

cmake --preset win-dev
cmake --build --preset win-dev
ctest --test-dir build/win-dev --output-on-failure
```

## 给后续 agent 的最短流程

1. 先读本文件、`DEVELOPMENT_CONTEXT.md` 和 `PROJECT_CONTEXT.md`，确认当前事实与最近进展。
2. 再读 `ARCHITECTURE.md`，确定修改属于哪个边界。
3. 搜索现有调用方和 DataHub topic，避免只改接口不改链路。
4. 修改后至少运行静态搜索、构建或局部测试；无法构建时记录原因。
5. 若行为、边界或完成状态变化，同步更新 `DEVELOPMENT_CONTEXT.md`、`docs/QUICKSTART.md` 和本目录文档。

## 重要原则

- 不把 README 中的规划项当成已实现功能。
- 不在 UI 线程执行不可控的网络或数据库长操作。
- 不为展示“设计模式”而增加没有真实调用方的抽象。
- 新模块必须有一个可验证的调用链、错误路径和测试入口。
