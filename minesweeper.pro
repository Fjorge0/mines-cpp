TEMPLATE += app
HEADERS += minesweeper.hpp
SOURCES += gui.cpp
TARGET = minesweeper.exe
CONFIG += c++23
QMAKE_CXXFLAGS += -std=c++23
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
