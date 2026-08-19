#pragma once

#include "datahub/QuoteData.h"

#include <QObject>

namespace fininsight::datahub {

class QuoteStream : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    ~QuoteStream() override = default;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void subscribe(const QString& symbol) = 0;
    virtual void unsubscribe(const QString& symbol) = 0;
    virtual bool isConnected() const = 0;

signals:
    void quoteReceived(const fininsight::datahub::QuoteData& quote);
    void statusChanged(const QString& status);
    void errorOccurred(const QString& error);
};

} // namespace fininsight::datahub
