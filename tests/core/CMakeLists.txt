cmake_minimum_required(VERSION 3.20)
project(FinInsightCoreTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(fininsight_core_tests
    core_tests.cpp
    ../../src/market/QuoteRules.cpp
    ../../src/simulation/Ledger.cpp
    ../../src/simulation/InvestmentExperiment.cpp
    ../../src/simulation/HistoricalPriceSeries.cpp
    ../../src/analysis/BehaviorAnalyzer.cpp
    ../../src/analysis/ReviewGenerator.cpp
    ../../src/trading/TradingTypes.cpp
    ../../src/trading/RiskEngine.cpp
    ../../src/trading/PaperExecutionGateway.cpp
    ../../src/market/SymbolResolver.cpp
    ../../src/trading/OrderService.cpp
)
target_include_directories(fininsight_core_tests PRIVATE ../../src)

enable_testing()
add_test(NAME fininsight_core_tests COMMAND fininsight_core_tests)
