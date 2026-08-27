#pragma once

#include "trading/TradingTypes.h"

namespace fininsight::notifications {
class NotificationService;

class TradingNotificationBridge final {
public:
    explicit TradingNotificationBridge(NotificationService& service) : service_(service) {}
    void onOrderChanged(const trading::Order& order);

private:
    NotificationService& service_;
};
}
