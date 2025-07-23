#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "AppLoadCoordinator.h"
#include "AppLibrary.h"
#include "AppLoad.h"
#include "management.h"
#include "library.h"
#include "icon.h"
#include "xovi.h"

extern "C" {
    void _xovi_construct() {
        loadIcon();
        qmlRegisterType<AppLoad>("net.asivery.AppLoad", 1, 0, "AppLoad");
        qmlRegisterType<AppLoadCoordinator>("net.asivery.AppLoad", 1, 0, "AppLoadCoordinator");
        qmlRegisterType<AppLoadLibrary>("net.asivery.AppLoad", 1, 0, "AppLoadLibrary");
        qmlRegisterType<AppLoadApplication>("net.asivery.AppLoad", 1, 0, "AppLoadApplication");
        appload::library::loadApplications();
        qt_resource_rebuilder$qmldiff_add_external_diff(r$apploadDiff, "AppLoad hooks in main UI");
    }
}
