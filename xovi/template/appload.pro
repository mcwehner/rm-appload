QT     += core gui qml quickcontrols2

TARGET = appload
TEMPLATE = lib
CONFIG += shared plugin no_plugin_name_prefix

xoviextension.target = xovi.cpp
xoviextension.commands = python3 $$(XOVI_REPO)/util/xovigen.py -o xovi.cpp -H xovi.h appload.xovi
xoviextension.depends = appload.xovi

QMAKE_EXTRA_TARGETS += xoviextension
PRE_TARGETDEPS += xovi.cpp

SOURCES += temporary/src/main.cpp xovi.cpp temporary/src/management.cpp temporary/src/AppLoad.cpp temporary/src/AppLoadCoordinator.cpp temporary/src/library.cpp temporary/src/libraryexternals.cpp temporary/src/qtfb/fbmanagement.cpp temporary/src/qtfb/FBController.cpp
HEADERS += temporary/src/AppLoad.h temporary/src/AppLoadCoordinator.h temporary/src/library.h temporary/src/AppLibrary.h \
            temporary/src/qtfb/FBController.h temporary/src/qtfb/fbmanagement.h

