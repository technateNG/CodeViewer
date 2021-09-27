cmake_minimum_required(VERSION 3.21)

project(CodeViewer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(QT NAMES Qt5 COMPONENTS Core Quick REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Quick REQUIRED)
find_package(Clang REQUIRED)

#include(FetchContent)
#FetchContent_Declare(
#    QuickQanava
#    GIT_REPOSITORY https://github.com/cneben/QuickQanava
#    GIT_TAG ab88d77ec62175b9fd499a154ffaf92f7bf23989
#    GIT_SUBMODULES_RECURSE ON
#    GIT_SHALLOW ON
#    GIT_PROGRESS ON
#)

#FetchContent_MakeAvailable(QuickQanava)

set(PROJECT_SOURCES
    src/main.cpp
    src/backend.cpp
    src/indexer.cpp
    qml/qml.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(gui MANUAL_FINALIZATION ${PROJECT_SOURCES})
else()
    if(ANDROID)
        add_library(gui SHARED ${PROJECT_SOURCES})
    else()
        add_executable(gui ${PROJECT_SOURCES})
    endif()
endif()

target_compile_definitions(
    gui PRIVATE
    $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>
)

target_link_libraries(
    gui PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Quick
    libclang
)

set_target_properties(gui PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.gui.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_import_qml_plugins(gui)
    qt_finalize_executable(gui)
endif()
