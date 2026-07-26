# FinInsight

> 基于 C++20 + Qt6 的桌面金融数据终端，兴趣驱动的个人项目。

[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus)](https://isocpp.org/)
[![Qt6](https://img.shields.io/badge/Qt-6.7-41CD52?logo=qt&logoColor=white)](https://www.qt.io/)
[![CMake](https://img.shields.io/badge/CMake-3.27-064F8C?logo=cmake)](https://cmake.org/)
[![SQLite](https://img.shields.io/badge/SQLite-3-003B57?logo=sqlite)](https://sqlite.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

---

## 关于项目

**FinInsight** 是一个基于 C++20 + Qt6 的桌面金融数据终端。项目源于对桌面软件开发和金融数据的兴趣，参考了一些优秀开源项目的架构思路，所有代码完全独立编写。

### 为什么做这个项目

- **练 C++20 工程能力**：模板、多线程、网络、数据库、发布订阅模式
- **学习大型 C++ 项目架构**：模块化设计、设计模式实践、构建系统
- **补桌面端技能**：Qt6 Widgets 开发、可拖拽面板、信号槽、自定义渲染

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

## 模块总览

| 模块 | 说明 | 状态 |
|------|------|------|
| 主窗口 + 可拖拽面板 | Qt ADS 实现可拖拽停靠布局 | ✅ |
| 全局配置管理 | AppConfig 单例，管理数据/缓存路径 | ✅ |
| SQLite 数据层 | WAL 模式 + Repository 模板 + 版本迁移 | ✅ |
| 多源数据管道 | Yahoo / EastMoney / Sina 竞争获取 | ✅ |
| DataHub 发布订阅 | 通配符匹配 + 线程安全推送 | ✅ |
| K 线图 + 技术指标 | 7 大指标 (MACD/RSI/KDJ/BOLL 等) | ✅ |
| DSL 表达式引擎 | 手写递归下降 Parser + AST 求值 | ✅ |
| 多源聚合器 | QtConcurrent 竞争 + 回退机制 | ✅ |
| 自选股 / 详情 / 组合 | StockList + Detail + Portfolio 面板 | ✅ |
| WebSocket 实时推送 | 实时行情（规划中） | 💡 |
| AI Agent 集成 | LLM 工具调用（规划中） | 💡 |

---

## License

MIT © 2026
