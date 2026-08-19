qt_add_executable(fininsight_qt_tests
    qt_tests.cpp
    ../../src/market/QuoteRules.cpp
    ../../src/network/HttpClient.cpp
    ../../src/datahub/QuoteAdapters.cpp
    ../../src/datahub/DataHub.cpp
    ../../src/datahub/Aggregator.cpp
)

target_include_directories(fininsight_qt_tests PRIVATE ../../src)
target_compile_definitions(fininsight_qt_tests PRIVATE
    FININSIGHT_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../fixtures/quotes"
)
target_link_libraries(fininsight_qt_tests PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Test
)

add_test(NAME fininsight_qt_tests COMMAND fininsight_qt_tests)
get_target_property(_qt_qmake Qt6::qmake IMPORTED_LOCATION)
cmake_path(GET _qt_qmake PARENT_PATH _qt_bin_dir)
set_tests_properties(fininsight_qt_tests PROPERTIES
    TIMEOUT 20
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${_qt_bin_dir}"
)

qt_add_executable(portfolio_panel_tests
    portfolio_panel_tests.cpp
    ../../src/market/QuoteRules.cpp
    ../../src/simulation/Ledger.cpp
    ../../src/panels/PortfolioPanel.h
    ../../src/panels/PortfolioPanel.cpp
)

target_include_directories(portfolio_panel_tests PRIVATE ../../src)
target_link_libraries(portfolio_panel_tests PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
)

add_test(NAME portfolio_panel_tests COMMAND portfolio_panel_tests)
set_tests_properties(portfolio_panel_tests PROPERTIES
    TIMEOUT 20
    ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:${_qt_bin_dir};QT_QPA_PLATFORM=set:offscreen"
)

qt_add_executable(experiment_panel_tests
    experiment_panel_tests.cpp
    ../../src/market/QuoteRules.cpp
    ../../src/simulation/Ledger.cpp
    ../../src/simulation/InvestmentExperiment.cpp
    ../../src/simulation/HistoricalPriceSeries.cpp
    ../../src/datahub/HistoricalPriceAdapter.cpp
    ../../src/panels/ExperimentPanel.h
    ../../src/panels/ExperimentPanel.cpp
)

target_include_directories(experiment_panel_tests PRIVATE ../../src)
target_link_libraries(experiment_panel_tests PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
)

add_test(NAME experiment_panel_tests COMMAND experiment_panel_tests)
set_tests_properties(experiment_panel_tests PROPERTIES
    TIMEOUT 20
    ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:${_qt_bin_dir};QT_QPA_PLATFORM=set:offscreen"
)

qt_add_executable(websocket_client_tests
    websocket_client_tests.cpp
    ../../src/network/WebSocketClient.h
    ../../src/network/WebSocketClient.cpp
    ../../src/datahub/DataHub.h
    ../../src/datahub/DataHub.cpp
    ../../src/datahub/QuoteStream.h
    ../../src/datahub/QuoteStreamService.h
    ../../src/datahub/QuoteStreamService.cpp
    ../../src/datahub/YahooWebSocketAdapter.h
    ../../src/datahub/YahooWebSocketAdapter.cpp
)

target_include_directories(websocket_client_tests PRIVATE ../../src)
target_link_libraries(websocket_client_tests PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::WebSockets
    Qt6::Test
)

add_test(NAME websocket_client_tests COMMAND websocket_client_tests)
set_tests_properties(websocket_client_tests PROPERTIES
    TIMEOUT 20
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${_qt_bin_dir}"
)

qt_add_executable(evidence_repository_tests
    evidence_repository_tests.cpp
    ../../src/analysis/BehaviorAnalyzer.cpp
    ../../src/analysis/ReviewGenerator.cpp
    ../../src/storage/Database.cpp
    ../../src/storage/EvidenceSnapshotRepository.cpp
    ../../src/storage/TradingOrderRepository.cpp
    ../../src/trading/TradingTypes.cpp
)

target_include_directories(evidence_repository_tests PRIVATE ../../src)
target_link_libraries(evidence_repository_tests PRIVATE
    Qt6::Core
    Qt6::Sql
    Qt6::Widgets
    Qt6::Test
)

add_test(NAME evidence_repository_tests COMMAND evidence_repository_tests)
set_tests_properties(evidence_repository_tests PROPERTIES
    TIMEOUT 20
    ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:${_qt_bin_dir};QT_QPA_PLATFORM=set:offscreen"
)

qt_add_executable(agent_review_panel_tests
    agent_review_panel_tests.cpp
    ../../src/analysis/BehaviorAnalyzer.cpp
    ../../src/analysis/ReviewGenerator.cpp
    ../../src/analysis/OpenAICompatibleReviewGenerator.cpp
    ../../src/network/HttpClient.cpp
    ../../src/storage/Database.cpp
    ../../src/storage/EvidenceSnapshotRepository.cpp
    ../../src/panels/AgentReviewPanel.cpp
)

target_include_directories(agent_review_panel_tests PRIVATE ../../src)
target_link_libraries(agent_review_panel_tests PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    Qt6::Network
    Qt6::Test
)

add_test(NAME agent_review_panel_tests COMMAND agent_review_panel_tests)
set_tests_properties(agent_review_panel_tests PROPERTIES
    TIMEOUT 20
    ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:${_qt_bin_dir};QT_QPA_PLATFORM=set:offscreen"
)

qt_add_executable(realtime_trading_panel_tests
    realtime_trading_panel_tests.cpp
    ../../src/trading/TradingTypes.cpp
    ../../src/trading/RiskEngine.cpp
    ../../src/trading/PaperExecutionGateway.cpp
    ../../src/trading/OrderService.cpp
    ../../src/panels/RealtimeTradingPanel.cpp
)
target_include_directories(realtime_trading_panel_tests PRIVATE ../../src)
target_link_libraries(realtime_trading_panel_tests PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test)
add_test(NAME realtime_trading_panel_tests COMMAND realtime_trading_panel_tests)
set_tests_properties(realtime_trading_panel_tests PROPERTIES
    TIMEOUT 20
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${_qt_bin_dir};QT_QPA_PLATFORM=set:offscreen"
)

qt_add_executable(agent_provider_tests
    agent_provider_tests.cpp
    ../../src/analysis/OpenAICompatibleReviewGenerator.cpp
    ../../src/network/HttpClient.cpp
)
target_include_directories(agent_provider_tests PRIVATE ../../src)
target_link_libraries(agent_provider_tests PRIVATE Qt6::Core Qt6::Network Qt6::Test)
add_test(NAME agent_provider_tests COMMAND agent_provider_tests)
set_tests_properties(agent_provider_tests PROPERTIES TIMEOUT 20 ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${_qt_bin_dir}")

qt_add_executable(alpaca_broker_client_tests
    alpaca_broker_client_tests.cpp
    ../../src/network/HttpClient.cpp
    ../../src/trading/TradingTypes.cpp
    ../../src/trading/AlpacaBrokerClient.cpp
)
target_include_directories(alpaca_broker_client_tests PRIVATE ../../src)
target_link_libraries(alpaca_broker_client_tests PRIVATE Qt6::Core Qt6::Network Qt6::Test)
add_test(NAME alpaca_broker_client_tests COMMAND alpaca_broker_client_tests)
set_tests_properties(alpaca_broker_client_tests PROPERTIES TIMEOUT 20 ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${_qt_bin_dir}")

qt_add_executable(notification_tests
    notification_tests.cpp
    ../../src/notifications/NotificationService.cpp
    ../../src/notifications/TradingNotificationBridge.cpp
    ../../src/trading/TradingTypes.cpp
    ../../src/panels/NotificationPanel.cpp
)
target_include_directories(notification_tests PRIVATE ../../src)
target_link_libraries(notification_tests PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test)
add_test(NAME notification_tests COMMAND notification_tests)
set_tests_properties(notification_tests PROPERTIES
    TIMEOUT 20
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${_qt_bin_dir};QT_QPA_PLATFORM=set:offscreen"
)

qt_add_executable(monitoring_tests
    monitoring_tests.cpp
    ../../src/monitoring/LatencyTracker.cpp
    ../../src/monitoring/HealthMonitor.cpp
    ../../src/panels/HealthPanel.cpp
)
target_include_directories(monitoring_tests PRIVATE ../../src)
target_link_libraries(monitoring_tests PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test)
add_test(NAME monitoring_tests COMMAND monitoring_tests)
set_tests_properties(monitoring_tests PROPERTIES TIMEOUT 20 ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${_qt_bin_dir};QT_QPA_PLATFORM=set:offscreen")
