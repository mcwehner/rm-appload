#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "AppLoadCoordinator.h"
#include "AppLibrary.h"
#include "AppLoad.h"
#include "management.h"
#include "library.h"
#include "log.h"

#include "qtfb/FBController.h"
#include "qtfb/fbmanagement.h"

#include <dlfcn.h>

int main(int argc, char *argv[])
{
    QGuiApplication a(argc, argv);
    QQmlApplicationEngine engine;
    appload::library::loadApplications();
    qtfb::management::start();

    qmlRegisterType<AppLoad>("net.asivery.AppLoad", 1, 0, "AppLoad");
    qmlRegisterType<AppLoadCoordinator>("net.asivery.AppLoad", 1, 0, "AppLoadCoordinator");
    qmlRegisterType<AppLoadLibrary>("net.asivery.AppLoad", 1, 0, "AppLoadLibrary");
    qmlRegisterType<AppLoadApplication>("net.asivery.AppLoad", 1, 0, "AppLoadApplication");
    qmlRegisterType<FBController>("net.asivery.Framebuffer", 1, 0, "FBController");
    engine.load(QUrl(QStringLiteral("./_start.qml")));

    return a.exec();
}
