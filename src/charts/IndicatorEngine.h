#pragma once

/// @file 技术指标计算引擎 — 纯 C++ 实现，零 Qt 依赖，可直接单独测试

#include <vector>
#include <cstddef>

namespace fininsight::charts {

// ── 结果结构体 ─────────────────────────────────────

struct MACDResult {
    std::vector<double> line;       // DIF  (快线)
    std::vector<double> signal;     // DEA  (慢线)
    std::vector<double> histogram;  // 柱状 (DIF - DEA) × 2
    bool empty() const { return line.empty(); }
};

struct BollingerResult {
    std::vector<double> upper;      // 上轨
    std::vector<double> middle;     // 中轨 (SMA)
    std::vector<double> lower;      // 下轨
    bool empty() const { return middle.empty(); }
};

struct KDJResult {
    std::vector<double> k;
    std::vector<double> d;
    std::vector<double> j;
    bool empty() const { return k.empty(); }
};

// ── 移动平均 ───────────────────────────────────────

/// 简单移动平均
std::vector<double> computeSMA(const std::vector<double>& data, int period);

/// 指数移动平均（递归：EMA_today = α × price + (1-α) × EMA_yesterday）
std::vector<double> computeEMA(const std::vector<double>& data, int period);

// ── 单线指标 ───────────────────────────────────────

/// 相对强弱指标（默认 14 日）
std::vector<double> computeRSI(const std::vector<double>& closes,
                                int period = 14);

/// 能量潮 (On-Balance Volume)
std::vector<double> computeOBV(const std::vector<double>& closes,
                                const std::vector<double>& volumes);

// ── 多线指标 ───────────────────────────────────────

/// MACD (12, 26, 9) — 最常用配置
MACDResult computeMACD(const std::vector<double>& closes,
                        int fast = 12, int slow = 26, int signalPeriod = 9);

/// 布林带 (20, 2.0)
BollingerResult computeBollinger(const std::vector<double>& closes,
                                  int period = 20, double multiplier = 2.0);

/// KDJ 随机指标 (9, 3, 3)
KDJResult computeKDJ(const std::vector<double>& highs,
                      const std::vector<double>& lows,
                      const std::vector<double>& closes,
                      int period = 9, int kPeriod = 3, int dPeriod = 3);

} // namespace fininsight::charts
