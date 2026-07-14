#include "charts/KLineChart.h"
#include "charts/IndicatorEngine.h"

#include <QDateTime>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPen>
#include <QCandlestickSet>
#include <QDebug>

namespace fininsight::charts {

// ── 构造 ────────────────────────────────────────────

KLineChart::KLineChart(QWidget* parent)
    : QChartView(parent)
{
    chart_ = new QChart();
    chart_->setTitle("");
    chart_->setAnimationOptions(QChart::SeriesAnimations);
    chart_->legend()->setVisible(true);
    chart_->legend()->setAlignment(Qt::AlignTop);
    chart_->legend()->setLabelColor(QColor("#586069"));

    // 浅色背景
    chart_->setBackgroundBrush(QBrush(QColor("#ffffff")));
    chart_->setPlotAreaBackgroundBrush(QBrush(QColor("#fafbfc")));
    chart_->setPlotAreaBackgroundVisible(true);

    setChart(chart_);
    setRenderHint(QPainter::Antialiasing, true);
    setDragMode(QGraphicsView::NoDrag);
    setRubberBand(QChartView::RectangleRubberBand);

    // 坐标轴 — 浅色网格
    axisX_ = new QDateTimeAxis();
    axisX_->setFormat("MM-dd");
    axisX_->setLabelsColor(QColor("#586069"));
    axisX_->setGridLineColor(QColor("#e8eaed"));
    axisX_->setLinePenColor(QColor("#d0d7de"));
    chart_->addAxis(axisX_, Qt::AlignBottom);

    axisY_ = new QValueAxis();
    axisY_->setLabelsColor(QColor("#586069"));
    axisY_->setGridLineColor(QColor("#e8eaed"));
    axisY_->setLinePenColor(QColor("#d0d7de"));
    chart_->addAxis(axisY_, Qt::AlignRight);
}

// ── 设置数据 ────────────────────────────────────────

void KLineChart::setData(const QVector<datahub::KLineData>& bars) {
    if (bars.isEmpty()) return;

    // 清理旧数据
    chart_->removeAllSeries();
    for (auto* ax : chart_->axes())
        chart_->removeAxis(ax);

    // 重建坐标轴
    axisX_ = new QDateTimeAxis();
    axisX_->setFormat("MM-dd");
    chart_->addAxis(axisX_, Qt::AlignBottom);

    axisY_ = new QValueAxis();
    chart_->addAxis(axisY_, Qt::AlignRight);

    // 创建 K 线序列
    candleSeries_ = new QCandlestickSeries();
    candleSeries_->setName("K-Line");
    candleSeries_->setIncreasingColor(QColor(200, 50, 50));   // 阳线红色
    candleSeries_->setDecreasingColor(QColor(50, 180, 50));   // 阴线绿色

    double minPrice = 1e18, maxPrice = -1e18;

    for (const auto& bar : bars) {
        QCandlestickSet* set = new QCandlestickSet(
            bar.open, bar.high, bar.low, bar.close);
        set->setTimestamp(dateToQDateTime(bar.date).toMSecsSinceEpoch());
        candleSeries_->append(set);

        minPrice = std::min(minPrice, bar.low);
        maxPrice = std::max(maxPrice, bar.high);
    }

    chart_->addSeries(candleSeries_);
    candleSeries_->attachAxis(axisX_);
    candleSeries_->attachAxis(axisY_);

    // 设定价格范围（留 5% 边距）
    double margin = (maxPrice - minPrice) * 0.05;
    axisY_->setRange(minPrice - margin, maxPrice + margin);

    chart_->setTitle(QString("%1  ·  %2 bars").arg(bars.first().symbol).arg(bars.size()));
    chart_->titleBrush().setColor(QColor("#5f6368"));
    QFont titleFont = chart_->titleFont();
    titleFont.setPointSize(10);
    titleFont.setWeight(QFont::Normal);
    chart_->setTitleFont(titleFont);
}

// ── 叠加均线 ────────────────────────────────────────

void KLineChart::addMA(int period, const QColor& color) {
    if (!candleSeries_) return;

    // 从现有 K 线提取收盘价
    auto sets = candleSeries_->sets();
    std::vector<double> closes;
    for (int i = 0; i < sets.size(); ++i) {
        auto* s = static_cast<QCandlestickSet*>(sets[i]);
        closes.push_back(s->close());
    }

    auto sma = computeSMA(closes, period);

    auto* series = new QLineSeries();
    series->setName(QString("MA%1").arg(period));
    QPen pen(color, 2);
    series->setPen(pen);

    for (int i = 0; i < static_cast<int>(sma.size()); ++i) {
        if (sma[i] > 0) {
            auto* s = static_cast<QCandlestickSet*>(sets[i]);
            series->append(s->timestamp(), sma[i]);
        }
    }

    chart_->addSeries(series);
    series->attachAxis(axisX_);
    series->attachAxis(axisY_);
}

void KLineChart::addEMA(int period, const QColor& color) {
    if (!candleSeries_) return;

    auto sets = candleSeries_->sets();
    std::vector<double> closes;
    for (int i = 0; i < sets.size(); ++i) {
        auto* s = static_cast<QCandlestickSet*>(sets[i]);
        closes.push_back(s->close());
    }

    auto ema = computeEMA(closes, period);

    auto* series = new QLineSeries();
    series->setName(QString("EMA%1").arg(period));
    QPen pen(color, 2, Qt::DashLine);
    series->setPen(pen);

    for (int i = 0; i < static_cast<int>(ema.size()); ++i) {
        if (ema[i] > 0) {
            auto* s = static_cast<QCandlestickSet*>(sets[i]);
            series->append(s->timestamp(), ema[i]);
        }
    }

    chart_->addSeries(series);
    series->attachAxis(axisX_);
    series->attachAxis(axisY_);
}

void KLineChart::addBollinger() {
    if (!candleSeries_) return;

    auto sets = candleSeries_->sets();
    std::vector<double> closes;
    for (int i = 0; i < sets.size(); ++i) {
        auto* s = static_cast<QCandlestickSet*>(sets[i]);
        closes.push_back(s->close());
    }

    auto boll = computeBollinger(closes, 20, 2.0);

    auto addLine = [&](const std::vector<double>& data,
                        const QString& name, const QColor& color) {
        auto* series = new QLineSeries();
        series->setName(name);
        series->setPen(QPen(color, 1, Qt::DotLine));
        for (int i = 0; i < static_cast<int>(data.size()); ++i) {
            if (data[i] > 0) {
                auto* s = static_cast<QCandlestickSet*>(sets[i]);
                series->append(s->timestamp(), data[i]);
            }
        }
        chart_->addSeries(series);
        series->attachAxis(axisX_);
        series->attachAxis(axisY_);
    };

    addLine(boll.upper,  "BOLL-Upper", QColor(180, 120, 220));
    addLine(boll.middle, "BOLL-Mid",   QColor(180, 180, 180));
    addLine(boll.lower,  "BOLL-Lower", QColor(180, 120, 220));
}

void KLineChart::clearIndicators() {
    // 移除所有非 K 线的 series
    auto all = chart_->series();
    for (auto* s : all) {
        if (s != candleSeries_) {
            chart_->removeSeries(s);
            delete s;
        }
    }
}

// ── 缩放 ────────────────────────────────────────────

void KLineChart::zoomIn() {
    chart_->zoom(1.3);
}

void KLineChart::zoomOut() {
    chart_->zoom(0.77);
}

void KLineChart::resetZoom() {
    chart_->zoomReset();
}

void KLineChart::wheelEvent(QWheelEvent* event) {
    if (event->angleDelta().y() > 0)
        chart_->zoom(1.1);
    else
        chart_->zoom(0.9);
}

// ── 拖拽平移 + 十字光标 ─────────────────────────────

void KLineChart::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::RightButton) {
        isDragging_ = true;
        lastMousePos_ = event->pos();
        setCursor(Qt::ClosedHandCursor);
        return;
    }
    QChartView::mousePressEvent(event);
}

void KLineChart::mouseMoveEvent(QMouseEvent* event) {
    if (isDragging_) {
        QPointF delta = event->pos() - lastMousePos_;
        chart_->scroll(-delta.x(), delta.y());
        lastMousePos_ = event->pos();
        return;
    }
    QChartView::mouseMoveEvent(event);
}

void KLineChart::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::RightButton && isDragging_) {
        isDragging_ = false;
        setCursor(Qt::ArrowCursor);
        return;
    }
    QChartView::mouseReleaseEvent(event);
}

// ── 辅助 ────────────────────────────────────────────

std::vector<double> KLineChart::extractCloses(
    const QVector<datahub::KLineData>& bars) const
{
    std::vector<double> result;
    result.reserve(bars.size());
    for (const auto& b : bars) result.push_back(b.close);
    return result;
}

QDateTime KLineChart::dateToQDateTime(const QString& dateStr) const {
    return QDateTime::fromString(dateStr, "yyyy-MM-dd");
}

} // namespace fininsight::charts
