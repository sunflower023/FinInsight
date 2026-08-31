#include "charts/KLineChart.h"
#include "charts/IndicatorEngine.h"

#include <QDateTime>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPen>
#include <QCandlestickSet>
#include <QGraphicsLineItem>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsScene>
#include <QDebug>

#include <limits>

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
    // 不启用框选缩放（左键只用于点选蜡烛，避免出现跟随鼠标的矩形框）
    setRubberBand(QChartView::NoRubberBand);

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

    // —— 十字光标（两条虚线）——
    crossHairV_ = new QGraphicsLineItem();
    crossHairV_->setPen(QPen(QColor("#9aa0a6"), 1, Qt::DashLine));
    crossHairV_->setZValue(10);
    crossHairV_->hide();
    scene()->addItem(crossHairV_);

    crossHairH_ = new QGraphicsLineItem();
    crossHairH_->setPen(QPen(QColor("#9aa0a6"), 1, Qt::DashLine));
    crossHairH_->setZValue(10);
    crossHairH_->hide();
    scene()->addItem(crossHairH_);

    // —— 选中蜡烛的竖线标记 ——
    selectionLine_ = new QGraphicsLineItem();
    selectionLine_->setPen(QPen(QColor("#1a73e8"), 2, Qt::SolidLine));
    selectionLine_->setZValue(9);
    selectionLine_->hide();
    scene()->addItem(selectionLine_);

    // —— 悬停提示框 ——
    tooltip_ = new QGraphicsSimpleTextItem();
    tooltip_->setBrush(QBrush(QColor("#1e1e1e")));
    tooltip_->setFont(QFont("Consolas", 9));
    tooltip_->setZValue(11);
    tooltip_->hide();
    scene()->addItem(tooltip_);
}

// ── 设置数据 ────────────────────────────────────────

void KLineChart::setData(const QVector<datahub::KLineData>& bars) {
    if (bars.isEmpty()) return;

    // 保存数据用于「坐标 → 蜡烛」映射与选时点
    bars_ = bars;
    selectedIndex_ = -1;

    // 清理旧数据
    chart_->removeAllSeries();
    for (auto* ax : chart_->axes())
        chart_->removeAxis(ax);
    tradeMarkers_ = nullptr;  // removeAllSeries 已删除，指针复位

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
    // 移除所有非 K 线的 series（保留 K 线与买卖标记）
    auto all = chart_->series();
    for (auto* s : all) {
        if (s != candleSeries_ && s != tradeMarkers_) {
            chart_->removeSeries(s);
            delete s;
        }
    }
}

// ── 缩放 ────────────────────────────────────────────

void KLineChart::applyZoom(double factor) {
    const double next = zoomFactor_ * factor;
    // 下界 1.0 = 原始尺寸，上界 64 倍，防止越缩越小或无限放大
    if (next < 1.0 || next > 64.0) return;
    chart_->zoom(factor);
    zoomFactor_ = next;
}

void KLineChart::zoomIn() {
    applyZoom(1.3);
}

void KLineChart::zoomOut() {
    applyZoom(1.0 / 1.3);
}

void KLineChart::resetZoom() {
    chart_->zoomReset();
    zoomFactor_ = 1.0;
}

void KLineChart::wheelEvent(QWheelEvent* event) {
    if (event->angleDelta().y() > 0)
        applyZoom(1.1);
    else
        applyZoom(1.0 / 1.1);
    event->accept();
}

// 双击左键：恢复到原始尺寸（解决放大后无法拉回的问题）
void KLineChart::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        resetZoom();
        event->accept();
        return;
    }
    QChartView::mouseDoubleClickEvent(event);
}

// ── 拖拽平移 + 十字光标 + 点击选时点 ────────────────

void KLineChart::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::RightButton) {
        isDragging_ = true;
        lastMousePos_ = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton) {
        pressPos_ = event->pos();
        leftPressed_ = true;
    }
    QChartView::mousePressEvent(event);
}

void KLineChart::mouseMoveEvent(QMouseEvent* event) {
    if (isDragging_) {
        QPointF delta = event->pos() - lastMousePos_;
        chart_->scroll(-delta.x(), delta.y());
        lastMousePos_ = event->pos();
        event->accept();
        return;
    }
    // 十字光标 + 悬停
    updateCrosshair(event->pos());
    const int index = barIndexAt(event->pos());
    if (index >= 0) emit barHovered(index, bars_[index]);
    QChartView::mouseMoveEvent(event);
}

void KLineChart::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::RightButton && isDragging_) {
        isDragging_ = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton && leftPressed_) {
        leftPressed_ = false;
        // 移动距离小于阈值视为「单击」→ 选中蜡烛
        const int dist = (event->pos() - pressPos_).manhattanLength();
        if (dist < 5) {
            const int index = barIndexAt(event->pos());
            if (index >= 0) {
                selectBar(index);
                emit barSelected(index, bars_[index]);
            } else {
                clearSelection();
            }
            event->accept();
            return;
        }
    }
    QChartView::mouseReleaseEvent(event);
}

void KLineChart::leaveEvent(QEvent* event) {
    if (crossHairV_) crossHairV_->hide();
    if (crossHairH_) crossHairH_->hide();
    if (tooltip_) tooltip_->hide();
    emit hoverLeft();
    QChartView::leaveEvent(event);
}

// ── 选时点交互辅助 ──────────────────────────────────

int KLineChart::barIndexAt(const QPointF& pos) const {
    if (!candleSeries_ || bars_.isEmpty()) return -1;
    const QPointF value = chart_->mapToValue(pos, candleSeries_);
    const qint64 targetTs = static_cast<qint64>(value.x());

    int best = -1;
    qint64 bestDiff = std::numeric_limits<qint64>::max();
    for (int i = 0; i < bars_.size(); ++i) {
        const qint64 ts = dateToQDateTime(bars_[i].date).toMSecsSinceEpoch();
        const qint64 diff = qAbs(ts - targetTs);
        if (diff < bestDiff) {
            bestDiff = diff;
            best = i;
        }
    }
    return best;
}

void KLineChart::updateCrosshair(const QPointF& pos) {
    if (!crossHairV_ || !crossHairH_) return;
    const QPointF scenePos = mapToScene(pos.toPoint());
    const QRectF plot = chart_->plotArea();
    if (!plot.contains(scenePos)) {
        crossHairV_->hide();
        crossHairH_->hide();
        if (tooltip_) tooltip_->hide();
        return;
    }
    crossHairV_->setLine(scenePos.x(), plot.top(), scenePos.x(), plot.bottom());
    crossHairH_->setLine(plot.left(), scenePos.y(), plot.right(), scenePos.y());
    crossHairV_->show();
    crossHairH_->show();

    if (tooltip_) {
        const int index = barIndexAt(pos);
        if (index >= 0 && index < bars_.size()) {
            const auto& bar = bars_[index];
            tooltip_->setText(QString("%1\nO:%2  H:%3\nL:%4  C:%5")
                .arg(bar.date)
                .arg(bar.open, 0, 'f', 2)
                .arg(bar.high, 0, 'f', 2)
                .arg(bar.low, 0, 'f', 2)
                .arg(bar.close, 0, 'f', 2));
            // 提示框放在光标上方，避免遮住下方的蜡烛
            tooltip_->setPos(scenePos.x() + 14, scenePos.y() - 64);
            tooltip_->show();
        } else {
            tooltip_->hide();
        }
    }
}

void KLineChart::updateSelectionLine() {
    if (!selectionLine_) return;
    if (selectedIndex_ < 0 || selectedIndex_ >= bars_.size()) {
        selectionLine_->hide();
        return;
    }
    const qint64 ts = dateToQDateTime(bars_[selectedIndex_].date).toMSecsSinceEpoch();
    const QPointF widgetPos = chart_->mapToPosition(QPointF(ts, 0), candleSeries_);
    const QPointF scenePos = mapToScene(widgetPos.toPoint());
    const QRectF plot = chart_->plotArea();
    selectionLine_->setLine(scenePos.x(), plot.top(), scenePos.x(), plot.bottom());
    selectionLine_->show();
}

void KLineChart::selectBar(int index) {
    if (index < 0 || index >= bars_.size()) {
        clearSelection();
        return;
    }
    selectedIndex_ = index;
    updateSelectionLine();
}

void KLineChart::clearSelection() {
    selectedIndex_ = -1;
    if (selectionLine_) selectionLine_->hide();
}

void KLineChart::highlightDate(const QString& date) {
    for (int i = 0; i < bars_.size(); ++i) {
        if (bars_[i].date == date) {
            selectBar(i);
            return;
        }
    }
}

void KLineChart::addTradeMarker(const QString& date, bool isBuy) {
    if (!chart_ || !axisX_ || !axisY_) return;
    const qint64 ts = dateToQDateTime(date).toMSecsSinceEpoch();
    if (!tradeMarkers_) {
        tradeMarkers_ = new QScatterSeries();
        tradeMarkers_->setMarkerSize(12.0);
        chart_->addSeries(tradeMarkers_);
        tradeMarkers_->attachAxis(axisX_);
        tradeMarkers_->attachAxis(axisY_);
    }
    // 用收盘价作为标记的 Y 位置（在选中蜡烛上）
    double price = 0.0;
    for (const auto& bar : bars_) {
        if (bar.date == date) { price = bar.close; break; }
    }
    tradeMarkers_->append(ts, price);
}

void KLineChart::clearTradeMarkers() {
    if (tradeMarkers_) {
        tradeMarkers_->clear();
    }
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
