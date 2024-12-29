QT     += core gui qml quickcontrols2

TARGET = appload
TEMPLATE = lib
CONFIG += shared

SOURCES += output/main.cpp management.cpp AppLoad.cpp AppLoadCoordinator.cpp library.cpp icon.cpp
HEADERS += AppLoad.h AppLoadCoordinator.h library.h AppLibrary.h

