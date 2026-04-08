cmake_minimum_required(VERSION 4.2)
project(StarWarsText)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if (WIN32)
    set(CMAKE_PREFIX_PATH "C:/Qt/6.7.3/msvc2022_64")
else ()
    set(CMAKE_PREFIX_PATH "$ENV{HOME}/Qt/6.10.1/gcc_64")
endif ()

find_package(Qt6 COMPONENTS
        Core
        Gui
        Multimedia
        Widgets
        REQUIRED)

add_executable(StarWarsText
        src/main.cpp
        src/CrawlContent.h
        src/TextIO.h
        src/TextIO.cpp
        src/CrawlWindow.h
        src/CrawlWindow.cpp
        src/MainWindow.h
        src/MainWindow.cpp
        src/StarMapWidget.h
        src/StarMapWidget.cpp
        src/StarsEditorWidget.h
        src/StarsEditorWidget.cpp
        src/LineNumberTextEdit.h
        src/LineNumberTextEdit.cpp
        resources/resources.qrc
)
target_link_libraries(StarWarsText
        Qt::Core
        Qt::Gui
        Qt::Multimedia
        Qt::Widgets
)

if (WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(DEBUG_SUFFIX)
    if (MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "d")
    endif ()
    set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
    if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
        set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
            set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        endif ()
    endif ()
    if (EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
    endif ()
    foreach (QT_LIB Core Gui Multimedia Widgets)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/bin/Qt6${QT_LIB}${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    endforeach (QT_LIB)
endif ()
