cmake_minimum_required(VERSION 3.27)
project(FinInsight VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ── 第三方依赖 (FetchContent) ──────────────────────────
include(FetchContent)

# Qt Advanced Docking System — 可拖拽面板布局
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC ON CACHE BOOL "" FORCE)
set(QT_VERSION_MAJOR 6 CACHE STRING "" FORCE)
FetchContent_Declare(
    QtADS
    GIT_REPOSITORY https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System.git
    GIT_TAG        87cffe5d40307c61141c88b88c10c5e7d181d880
    GIT_SHALLOW    TRUE
    UPDATE_DISCONNECTED TRUE
)
set(ADS_VERSION "4.5.0" CACHE STRING "" FORCE)
# Pre-set module path so include(Versioning) in qtads src/CMakeLists succeeds
list(APPEND CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}/_deps/qtads-src/cmake/modules")
FetchContent_MakeAvailable(QtADS)
set_target_properties(qtadvanceddocking-qt6 PROPERTIES UNITY_BUILD OFF)

# nlohmann/json — JSON 解析
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(json)

# ── Qt6 ────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Widgets Charts Network Sql)

# ── 子目录 ─────────────────────────────────────────────
add_subdirectory(src)
