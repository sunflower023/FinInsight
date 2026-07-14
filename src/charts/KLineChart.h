#pragma once

#include "datahub/QuoteData.h"

#include <QChartView>
#include <QChart>
#include <QCandlestickSeries>
#include <QLineSeries>
#include <QValueAxis>
#include <QDateTimeAxis>
#include <QVector>

namespace fininsight::charts {

/**
 * @brief K 线图表 Widget — 基于 Qt Charts
 *
 * 支持：
 *   - K 线蜡烛图
 *   - MA 均线叠加（MA5/MA10/MA20/MA60）
 *   - 鼠标滚轮缩放 + 拖拽平移
 *   - 十字光标
 *
 * 使用：
 *   auto* chart = new KLineChart();
 *   chart->setData(bars);            // 设置 K 线数据
 *   chart->addMA(5);                 // 叠加 5 日均线
 *   chart->addMA(20);                // 叠加 20 日均线
 */
class KLineChart : public QChartView {
    Q_OBJECT

public:
    explicit KLineChart(QWidget* parent = nullptr);

    /// 设置 K 线数据并重绘
    void setData(const QVector<datahub::KLineData>& bars);

    /// 叠加移动平均线
    void addMA(int period, const QColor& color = Qt::blue);

    /// 叠加指数移均线
    void addEMA(int period, const QColor& color = Qt::red);

    /// 叠加布林带（20, 2.0）
    void addBollinger();

    /// 清除所有指标线（保留 K 线）
    void clearIndicators();

public slots:
    void zoomIn();
    void zoomOut();
    void resetZoom();

protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    // 从 KLineData 提取 closes 到 std::vector（跨类型转换）
    std::vector<double> extractCloses(const QVector<datahub::KLineData>& bars) const;
    QDateTime dateToQDateTime(const QString& dateStr) const;

    QChart* chart_;
    QCandlestickSeries* candleSeries_ = nullptr;
    QDateTimeAxis* axisX_  = nullptr;
    QValueAxis*    axisY_  = nullptr;
    QPointF lastMousePos_;
    bool isDragging_ = false;
};

} // namespace fininsight::charts
