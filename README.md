# FinInsight

> 从 85 万行工业级 C++20 开源项目中提炼骨架，独立重构的桌面金融数据终端。

[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus)](https://isocpp.org/)
[![Qt6](https://img.shields.io/badge/Qt-6.7-41CD52?logo=qt&logoColor=white)](https://www.qt.io/)
[![CMake](https://img.shields.io/badge/CMake-3.27-064F8C?logo=cmake)](https://cmake.org/)
[![SQLite](https://img.shields.io/badge/SQLite-3-003B57?logo=sqlite)](https://sqlite.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

---

## 关于项目

**FinInsight** 是一个基于 C++20 + Qt6 的桌面金融数据终端，参考 [FinceptTerminal](https://github.com/Fincept-Corporation/FinceptTerminal)（~85 万行开源项目）的架构设计，提取核心骨架并完全独立实现。

### 为什么做这个项目

- **练 C++20 工程能力**：模板、多线程、网络、数据库、发布订阅模式
- **理解大型 C++ 项目架构**：从 85 万行项目中学习模块划分和设计模式
- **补桌面端技能**：Qt6 Widgets 开发、可拖拽面板、信号槽、自定义渲染

### 不是 Fork，是 Rebuild

本项目为独立 Coding，仅参考原始项目的模块设计思路，所有代码自行编写。

---

## 技术栈

| 层级 | 技术 |
|------|------|
| 语言 | C++20 |
| UI 框架 | Qt 6.7（Widgets + Charts + Network + Sql）|
| 构建系统 | CMake 3.27 + Ninja |
| 数据库 | SQLite（WAL 模式） |
| 包管理 | CMake FetchContent（Qt ADS / nlohmann json） |
| 数据源 | Yahoo Finance / AkShare |
| 平台 | Windows x64（可扩展 Linux/macOS） |

---

## 架构

```
FinInsight/
├── src/
│   ├── main.cpp              # 应用入口
│   ├── app/                  # 主窗口与面板管理
│   ├── core/                 # 核心抽象层（EventBus / DataHub / AppConfig）
│   ├── storage/              # 数据持久化（SQLite / Repository 模板 / 迁移）
│   ├── datahub/              # 数据管道（发布订阅 / 数据源生产者）
│   ├── network/              # 网络通信（HTTP / WebSocket）
│   ├── charts/               # 图表渲染（K 线 / 技术指标）
│   └── panels/               # 业务面板（自选股 / 详情 / 投资组合）
└── resources/
    └── icons/
```

**设计模式：** Singleton / Repository / Pub-Sub / Strategy / Producer-Consumer

---

## 构建

### 环境要求

- Visual Studio 2022 17.10+（MSVC 19.40+）
- Qt 6.7+（需安装 MSVC 2022 64-bit 套件）
- CMake 3.27+ / Ninja（VS 2022 自带）
- Python 3.11+（可选，数据分析用）

### 构建步骤

```powershell
# 1. 克隆
git clone https://github.com/你的用户名/FinInsight.git
cd FinInsight

# 2. 修改 CMakePresets.json 中的 CMAKE_PREFIX_PATH 指向你的 Qt 安装路径

# 3. 构建
.\build.bat

# 4. 运行
.\build\win-dev\src\FinInsight.exe
```

> 中国大陆用户需配置 GitHub 代理以拉取 FetchContent 依赖。

---

## 功能路线

| 阶段 | 模块 | 状态 |
|------|------|------|
| P0 | 主窗口 + 可拖拽面板 | ✅ 已完成 |
| P0 | 全局配置管理 | ✅ 已完成 |
| P1 | SQLite 数据层 + Repository 模板 | 🔨 开发中 |
| P1 | Yahoo Finance 数据源接入 | 📋 待开发 |
| P2 | DataHub 发布订阅数据管道 | 📋 待开发 |
| P2 | K 线图 + 技术指标渲染 | 📋 待开发 |
| P3 | 自选股管理 / 股票详情面板 | 📋 待开发 |
| P3 | 投资组合模拟 / 收益曲线 | 📋 待开发 |
| 扩展 | WebSocket 实时行情推送 | 💡 未来 |
| 扩展 | AI Agent 集成（LLM 工具调用） | 💡 未来 |

---

## License

MIT © 2026
