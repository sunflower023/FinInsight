# Roadmap

## P0：先让基础可信

- 建立 Catch2/CTest 测试目标，覆盖 IndicatorEngine、Lexer/Parser、DataHub。
- 修复 CMakePresets 的本机路径问题，记录 Qt/编译器版本。
- 为网络请求增加统一错误类型、取消和超时语义。
- 明确 UI 加载状态、重试和无网络时的缓存行为。

## P1：完成一条有含金量的业务闭环

- DSL 绑定历史 K 线，按时间序列求值。
- 增加模拟成交、持仓、收益率和最大回撤计算。
- 将交易记录和持仓迁移到 SQLite。
- 用异步多源请求实现真正的 first-valid + 取消剩余任务。

## P2：工程化

- 增加 CI（Windows 构建、单元测试、格式检查）。
- 增加离线 fixture，避免测试依赖 Yahoo/EastMoney 在线服务。
- 对指标计算和 JSON 解析增加基准测试与故障注入。
- 将 UI、数据源和策略引擎拆成更容易独立测试的 target。
