# Agent 开发入口

本目录是 FinInsight 的持续开发知识库，给后续 agent 或新加入的开发者使用。这里记录“代码当前是什么状态”和“修改时应遵守什么边界”，不把规划写成已完成事实。

## 文件索引

- [`PROJECT_CONTEXT.md`](PROJECT_CONTEXT.md)：项目定位、真实完成度、关键风险和面试口径。
- [`ARCHITECTURE.md`](ARCHITECTURE.md)：模块边界、依赖方向、数据流和线程模型。
- [`DEVELOPMENT_RULES.md`](DEVELOPMENT_RULES.md)：提交前检查、代码风格、测试和文档要求。
- [`ROADMAP.md`](ROADMAP.md)：按优先级排列的后续迭代路线。
- [`DEVELOPMENT_CONTEXT.md`](DEVELOPMENT_CONTEXT.md)：面向后续会话的实时开发上下文、开发记录和下一步入口。

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
