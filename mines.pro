TEMPLATE += app

# Files
HEADERS += $$files(include/*.hpp)
SOURCES += src/gui.cpp
RESOURCES += res/main.qrc

# Executable Name
TARGET = mines.exe

# Specify C++ version
CONFIG += c++23
QMAKE_CXXFLAGS += -std=c++23
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
