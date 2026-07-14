#include "charts/IndicatorEngine.h"

#include <cmath>
#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace fininsight::charts {

// ═══════════════════════════════════════════════════════
//  移动平均
// ═══════════════════════════════════════════════════════

std::vector<double> computeSMA(const std::vector<double>& data, int period) {
    if (period <= 0 || data.empty()) return {};

    std::vector<double> result(data.size(), 0.0);

    for (size_t i = 0; i < data.size(); ++i) {
        if (static_cast<int>(i) < period - 1) {
            result[i] = 0.0;  // 不足周期，填 0
            continue;
        }
        double sum = 0.0;
        for (int j = 0; j < period; ++j) {
            sum += data[i - j];
        }
        result[i] = sum / period;
    }
    return result;
}

std::vector<double> computeEMA(const std::vector<double>& data, int period) {
    if (period <= 0 || data.empty()) return {};

    std::vector<double> result(data.size(), 0.0);
    const double alpha = 2.0 / (period + 1.0);

    // 第一个有效值用 SMA 初始化
    result[period - 1] = computeSMA(data, period)[period - 1];

    for (size_t i = period; i < data.size(); ++i) {
        result[i] = alpha * data[i] + (1.0 - alpha) * result[i - 1];
    }
    return result;
}

// ═══════════════════════════════════════════════════════
//  单线指标
// ═══════════════════════════════════════════════════════

std::vector<double> computeRSI(const std::vector<double>& closes, int period) {
    if (closes.empty() || period <= 0) return {};

    std::vector<double> result(closes.size(), 0.0);

    double avgGain = 0.0, avgLoss = 0.0;

    // 第一个周期用简单平均初始化
    for (int i = 1; i <= period && i < static_cast<int>(closes.size()); ++i) {
        double diff = closes[i] - closes[i - 1];
        if (diff > 0) avgGain += diff;
        else          avgLoss -= diff;
    }
    avgGain /= period;
    avgLoss /= period;

    if (avgLoss == 0.0) result[period] = 100.0;
    else result[period] = 100.0 - (100.0 / (1.0 + avgGain / avgLoss));

    // 后续用 Wilder's Smoothing
    for (size_t i = period + 1; i < closes.size(); ++i) {
        double diff = closes[i] - closes[i - 1];
        double gain = (diff > 0) ? diff : 0.0;
        double loss = (diff < 0) ? -diff : 0.0;

        avgGain = (avgGain * (period - 1) + gain) / period;
        avgLoss = (avgLoss * (period - 1) + loss) / period;

        if (avgLoss == 0.0) result[i] = 100.0;
        else result[i] = 100.0 - (100.0 / (1.0 + avgGain / avgLoss));
    }
    return result;
}

std::vector<double> computeOBV(const std::vector<double>& closes,
                                const std::vector<double>& volumes) {
    size_t n = std::min(closes.size(), volumes.size());
    std::vector<double> result(n, 0.0);

    result[0] = volumes[0];
    for (size_t i = 1; i < n; ++i) {
        if (closes[i] > closes[i - 1])
            result[i] = result[i - 1] + volumes[i];
        else if (closes[i] < closes[i - 1])
            result[i] = result[i - 1] - volumes[i];
        else
            result[i] = result[i - 1];
    }
    return result;
}

// ═══════════════════════════════════════════════════════
//  多线指标
// ═══════════════════════════════════════════════════════

MACDResult computeMACD(const std::vector<double>& closes,
                        int fast, int slow, int signalPeriod) {
    MACDResult r;
    if (closes.empty()) return r;

    auto emaFast   = computeEMA(closes, fast);
    auto emaSlow   = computeEMA(closes, slow);

    size_t start = std::max({0, slow - 1, static_cast<int>(closes.size()) - 1});
    // 从第一个两个 EMA 都有效的位置开始
    if (closes.size() <= static_cast<size_t>(slow)) return r;

    r.line.resize(closes.size(), 0.0);
    for (size_t i = slow - 1; i < closes.size(); ++i) {
        r.line[i] = emaFast[i] - emaSlow[i];
    }

    // Signal 是 line 的 EMA(9)
    r.signal = computeEMA(r.line, signalPeriod);

    r.histogram.resize(closes.size(), 0.0);
    for (size_t i = 0; i < closes.size(); ++i) {
        r.histogram[i] = (r.line[i] - r.signal[i]) * 2.0;
    }
    return r;
}

BollingerResult computeBollinger(const std::vector<double>& closes,
                                  int period, double multiplier) {
    BollingerResult r;
    if (closes.empty() || period <= 0) return r;

    r.middle = computeSMA(closes, period);
    r.upper.resize(closes.size(), 0.0);
    r.lower.resize(closes.size(), 0.0);

    for (size_t i = 0; i < closes.size(); ++i) {
        if (static_cast<int>(i) < period - 1) continue;

        // 计算 period 内的标准差
        double sum = 0.0, sumSq = 0.0;
        for (int j = 0; j < period; ++j) {
            double val = closes[i - j];
            sum   += val;
            sumSq += val * val;
        }
        double mean = sum / period;
        double variance = sumSq / period - mean * mean;
        double stddev = std::sqrt(std::max(0.0, variance));

        r.upper[i] = r.middle[i] + multiplier * stddev;
        r.lower[i] = r.middle[i] - multiplier * stddev;
    }
    return r;
}

KDJResult computeKDJ(const std::vector<double>& highs,
                      const std::vector<double>& lows,
                      const std::vector<double>& closes,
                      int period, int kPeriod, int dPeriod) {
    KDJResult r;
    size_t n = std::min({highs.size(), lows.size(), closes.size()});
    if (n == 0) return r;

    r.k.resize(n, 50.0);
    r.d.resize(n, 50.0);
    r.j.resize(n, 50.0);

    for (size_t i = period - 1; i < n; ++i) {
        // 找 period 内的最高价和最低价
        double highest = *std::max_element(highs.begin() + i - period + 1,
                                            highs.begin() + i + 1);
        double lowest  = *std::min_element(lows.begin() + i - period + 1,
                                            lows.begin() + i + 1);

        // RSV = (close - low) / (high - low) × 100
        double rsv = (highest == lowest) ? 50.0
            : (closes[i] - lowest) / (highest - lowest) * 100.0;

        if (i == static_cast<size_t>(period - 1)) {
            r.k[i] = 50.0;
            r.d[i] = 50.0;
        }

        // K = 2/3 × prev_K + 1/3 × RSV
        r.k[i] = (2.0 * r.k[i - 1] + rsv) / 3.0;
        // D = 2/3 × prev_D + 1/3 × K
        r.d[i] = (2.0 * r.d[i - 1] + r.k[i]) / 3.0;
        // J = 3K - 2D
        r.j[i] = 3.0 * r.k[i] - 2.0 * r.d[i];
    }
    return r;
}

} // namespace fininsight::charts
