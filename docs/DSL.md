# DSL 模块文档

> 策略条件编译器：词法分析 → 递归下降解析 → 语义求值

---

## 一、模块定位

允许用户用自然表达式定义策略触发条件，无需写代码：

```
输入: "MACD > Signal AND RSI < 30 AND Close > MA(20)"
输出: 1.0 (触发) 或 0.0 (不触发)
```

---

## 二、架构

```
用户输入字符串 "MACD > Signal AND RSI < 30"
        │
        ▼
┌────────────────┐
│    Lexer       │  词法分析：字符流 → Token 序列
│  tokenize()    │
└───────┬────────┘
        │  [ID(MACD), >, ID(Signal), AND, ID(RSI), <, NUM(30)]
        ▼
┌────────────────┐
│    Parser      │  语法分析：Token 序列 → AST 抽象语法树
│  parse()       │  递归下降法（Recursive Descent）
└───────┬────────┘
        │          AND
        │         /    \
        │      cmp(>)   cmp(<)
        │      /   \     /   \
        │   MACD Signal RSI  30
        ▼
┌────────────────┐
│   Evaluator    │  语义求值：遍历 AST，查 IndicatorEngine
│  evaluate()    │  MACD > Signal? → 查实时 MACD 值 → 比较
└───────┬────────┘
        │
        ▼
   1.0 (触发) / 0.0 (不触发)
```

---

## 三、Token 类型

| Token | 类型 | 示例 |
|-------|------|------|
| 标识符 | `Identifier` | `MACD`, `Close`, `RSI` |
| 数字 | `Number` | `30`, `20` |
| 运算符 | `>`, `<`, `>=`, `<=`, `==`, `!=`, `AND`, `OR` | |
| 括号 | `(`, `)` | |

---

## 四、语法规则 (EBNF)

```
expression  → orExpr
orExpr      → andExpr ("OR" andExpr)*
andExpr     → cmpExpr ("AND" cmpExpr)*
cmpExpr     → atom (">" | "<" | ">=" | "<=" | "==" | "!=") atom
atom        → NUMBER
            | IDENTIFIER
            | IDENTIFIER "(" NUMBER ")"      // MA(20), RSI(14)
            | "(" expression ")"
```

**优先级从低到高：** OR < AND < 比较运算符 < 函数调用

---

## 五、支持的标识符

| 标识符 | 类型 | 含义 | 数据来源 |
|--------|------|------|---------|
| `Close` | 内置 | 最新收盘价 | K 线收盘价数组 |
| `Open` | 内置 | 最新开盘价 | K 线开盘价数组 |
| `High` | 内置 | 最高价 | K 线最高价数组 |
| `Low` | 内置 | 最低价 | K 线最低价数组 |
| `Volume` | 内置 | 成交量 | K 线成交量数组 |
| `MACD` | 内置 | MACD DIF 线最新值 | IndicatorEngine |
| `Signal` | 内置 | MACD Signal 线最新值 | IndicatorEngine |
| `RSI` | 内置 | RSI(14) 值 | IndicatorEngine |
| `BOLL_U` / `BOLL_M` / `BOLL_L` | 内置 | 布林带 | IndicatorEngine |
| `MA(n)` | 函数 | n 日均线 | IndicatorEngine::computeSMA |
| `EMA(n)` | 函数 | n 日指数均线 | IndicatorEngine::computeEMA |
| `RSI(n)` | 函数 | RSI(n) | IndicatorEngine::computeRSI |

---

## 六、使用示例

```cpp
#include "dsl/Lexer.h"
#include "dsl/Parser.h"
#include "dsl/Evaluator.h"

using namespace fininsight::dsl;

// 1. 设置 K 线数据
Evaluator eval;
eval.setData(opens, highs, lows, closes, volumes);

// 2. 评估表达式
auto result = eval.evaluate("MACD > Signal AND RSI < 30");
// result.ok    = true
// result.value = 1.0  ← MACD 金叉 + RSI 超卖，触发买入信号

// 更多示例：
eval.evaluate("Close > MA(20)");              // 收盘价突破 20 日均线
eval.evaluate("RSI(14) > 70");                // RSI 超买
eval.evaluate("RSI < 30 OR MACD > Signal");   // 组合条件
```

---

## 七、文件结构

```
src/dsl/
├── Token.h          ← Token 类型枚举 + 结构体
├── Lexer.h / .cpp   ← 词法分析器（~60 行）
├── Parser.h / .cpp   ← AST 节点 + 递归下降解析器（~120 行）
└── Evaluator.h / .cpp ← AST 求值器（~130 行）
```

**零 Qt 依赖（除 QString/调试），纯 C++20 std::variant。**
