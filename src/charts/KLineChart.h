#pragma once

#include "datahub/QuoteData.h"

#include <QChartView>
#include <QChart>
#include <QCandlestickSeries>
#include <QLineSeries>
#include <QScatterSeries>
#include <QValueAxis>
#include <QDateTimeAxis>
#include <QVector>

class QGraphicsLineItem;
class QGraphicsSimpleTextItem;

namespace fininsight::charts {

/**
 * @brief K 线图表 Widget — 基于 Qt Charts
 *
 * 支持：
 *   - K 线蜡烛图
 *   - MA 均线叠加（MA5/MA10/MA20/MA60）
 *   - 鼠标滚轮缩放 + 拖拽平移
 *   - 左键单击选中某根蜡烛（选时点）
 *   - 十字光标 + 悬停提示
 *   - 买卖箭头标记
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

    /// 当前图表持有的 K 线数据（供策略体检等外部逻辑只读使用）
    const QVector<datahub::KLineData>& bars() const { return bars_; }

    /// 叠加移动平均线
    void addMA(int period, const QColor& color = Qt::blue);

    /// 叠加指数移均线
    void addEMA(int period, const QColor& color = Qt::red);

    /// 叠加布林带（20, 2.0）
    void addBollinger();

    /// 清除所有指标线（保留 K 线）
    void clearIndicators();

    /// 程序化选中某根蜡烛（index 为 bars 下标，-1 清除选中）
    void selectBar(int index);
    /// 清除选中标记
    void clearSelection();
    /// 当前选中的蜡烛下标（-1 表示未选中）
    int selectedIndex() const { return selectedIndex_; }
    /// 按日期高亮对应的蜡烛（date 形如 "yyyy-MM-dd"）
    void highlightDate(const QString& date);

    /// 叠加买卖箭头标记（date 形如 "yyyy-MM-dd"，isBuy=true 买红↑ / false 卖绿↓）
    void addTradeMarker(const QString& date, bool isBuy);
    /// 清除所有买卖标记
    void clearTradeMarkers();

signals:
    /// 左键单击选中某根蜡烛
    void barSelected(int index, datahub::KLineData bar);
    /// 鼠标悬停在某根蜡烛上
    void barHovered(int index, datahub::KLineData bar);
    /// 鼠标离开图表
    void hoverLeft();

public slots:
    void zoomIn();
    void zoomOut();
    void resetZoom();

protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    // 从 KLineData 提取 closes 到 std::vector（跨类型转换）
    std::vector<double> extractCloses(const QVector<datahub::KLineData>& bars) const;
    QDateTime dateToQDateTime(const QString& dateStr) const;
    // 带边界的缩放：限制在 [1.0, 64.0]，避免越缩越小或无限放大
    void applyZoom(double factor);
    // 鼠标坐标 → 最近的蜡烛下标（-1 表示无）
    int barIndexAt(const QPointF& pos) const;
    // 更新十字光标与悬停提示框
    void updateCrosshair(const QPointF& pos);
    // 更新选中竖线位置
    void updateSelectionLine();

    QChart* chart_;
    QCandlestickSeries* candleSeries_ = nullptr;
    QDateTimeAxis* axisX_  = nullptr;
    QValueAxis*    axisY_  = nullptr;
    QPointF lastMousePos_;
    bool isDragging_ = false;
    double zoomFactor_ = 1.0;

    // —— 选时点交互 ——
    QVector<datahub::KLineData> bars_;
    int selectedIndex_ = -1;
    QPointF pressPos_;
    bool leftPressed_ = false;

    QGraphicsLineItem* crossHairV_ = nullptr;
    QGraphicsLineItem* crossHairH_ = nullptr;
    QGraphicsSimpleTextItem* tooltip_ = nullptr;
    QGraphicsLineItem* selectionLine_ = nullptr;
    QScatterSeries* tradeMarkers_ = nullptr;
};

} // namespace fininsight::charts
