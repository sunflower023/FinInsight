set(SOURCES
    main.cpp
    app/MainWindow.h
    app/MainWindow.cpp
    core/AppConfig.h
    core/AppConfig.cpp
    storage/Migration.h
    storage/Database.h
    storage/Database.cpp
    storage/BaseRepository.h
    storage/StockRepository.h
    storage/StockRepository.cpp
    storage/migrations/V001_Initial.h
    network/HttpClient.h
    network/HttpClient.cpp
    datahub/QuoteData.h
    datahub/DataHub.h
    datahub/DataHub.cpp
    datahub/YahooProducer.h
    datahub/YahooProducer.cpp
    datahub/EastMoneyProducer.h
    datahub/EastMoneyProducer.cpp
    charts/IndicatorEngine.h
    charts/IndicatorEngine.cpp
    charts/KLineChart.h
    charts/KLineChart.cpp
    panels/StockSearchBar.h
    panels/StockSearchBar.cpp
    panels/StockListPanel.h
    panels/StockListPanel.cpp
    panels/DetailPanel.h
    panels/DetailPanel.cpp
    panels/PortfolioPanel.h
    panels/PortfolioPanel.cpp
    dsl/Token.h
    dsl/Lexer.h
    dsl/Lexer.cpp
    dsl/Parser.h
    dsl/Parser.cpp
    dsl/Evaluator.h
    dsl/Evaluator.cpp
    datahub/Aggregator.h
    datahub/Aggregator.cpp
)

qt_add_executable(FinInsight ${SOURCES})

target_include_directories(FinInsight PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(FinInsight PRIVATE
    Qt6::Widgets
    Qt6::Charts
    Qt6::Network
    Qt6::Sql
    Qt6::Concurrent
    qtadvanceddocking-qt6
    nlohmann_json::nlohmann_json
)

# ── 复制 Qt DLL 到输出目录 (仅 Windows) ────────────────
if(WIN32)
    set_target_properties(FinInsight PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    # 自动复制 Qt DLL + 插件到输出目录
    get_target_property(_qt_bin Qt6::qmake IMPORTED_LOCATION)
    cmake_path(GET _qt_bin PARENT_PATH _qt_bin_dir)
    find_program(WINDEPLOYQT windeployqt HINTS "${_qt_bin_dir}" REQUIRED)
    add_custom_command(TARGET FinInsight POST_BUILD
        COMMAND "${WINDEPLOYQT}" "$<TARGET_FILE:FinInsight>"
            --no-translations --no-system-d3d-compiler
            --no-opengl-sw
        COMMENT "Deploying Qt DLLs..."
    )
endif()
