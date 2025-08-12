#pragma once

#include <QObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QString>
#include <QImage>
#include <QJsonDocument>
#include <QPainter>
#include <QJsonObject>
#include <QJsonValue>
#include <QQuickPaintedItem>

#include "library.h"

#define INTERNAL 0
#define EXTERNAL_NOGUI 1
#define EXTERNAL_QTFB 2
#define EXTERNAL_TERMINAL 3

class AppLoadApplication : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString icon READ icon CONSTANT)
    Q_PROPERTY(bool supportsScaling READ supportsScaling)
    Q_PROPERTY(bool canHaveMultipleFrontends READ canHaveMultipleFrontends)
    Q_PROPERTY(int externalType READ externalType) // 0 - not external, 1 - external (non-graphics), 2 - external (qtfb), 3 - terminal (literm)

public:
    explicit AppLoadApplication(QObject *parent = nullptr)
        : QObject(parent) {}
    AppLoadApplication(const QString &id, const QString &name, const QString &icon, bool supportsScaling, bool canHaveMultipleFrontends, int externalType, QObject *parent = nullptr)
        : QObject(parent), _id(id), _name(name), _icon(icon), _supportsScaling(supportsScaling), _canHaveMultipleFrontends(canHaveMultipleFrontends), _externalType(externalType) {}

    QString id() const { return _id; }
    QString name() const { return _name; }
    QString icon() const { return _icon; }
    bool supportsScaling() const { return _supportsScaling; }
    bool canHaveMultipleFrontends() const { return _canHaveMultipleFrontends; }
    int externalType() const { return _externalType; }

private:
    QString _id;
    QString _name;
    QString _icon;
    bool _supportsScaling;
    bool _canHaveMultipleFrontends;
    int _externalType;
};

class AppLoadLibrary : public QObject {
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<AppLoadApplication> applications READ applications NOTIFY applicationsChanged)

public:
    explicit AppLoadLibrary(QObject *parent = nullptr) : QObject(parent) { }

    QQmlListProperty<AppLoadApplication> applications() {
        loadList();
        return QQmlListProperty<AppLoadApplication>(this, this,
                                                    &AppLoadLibrary::appendApplication,
                                                    &AppLoadLibrary::applicationCount,
                                                    &AppLoadLibrary::applicationAt,
                                                    &AppLoadLibrary::clearApplications);
    }

    Q_INVOKABLE int reloadList() {
        int count = appload::library::loadApplications();
        emit applicationsChanged();
        return count;
    }

    Q_INVOKABLE bool isFrontendRunningFor(const QString &appID) {
        auto app = appload::library::get(appID);
        if(app) {
            return app->isFrontendRunning();
        } else {
            return false;
        }
    }

    Q_INVOKABLE bool launchExternal(const QString &appID) {
        auto ref = appload::library::getExternals().find(appID);
        if(ref != appload::library::getExternals().end()) {
            ref->second->launch();
            return true;
        }

        return false;
    }

    void loadList() {
        clearApplications();
        for (const auto &entry : appload::library::getRef()) {
            _applications.append(new AppLoadApplication(entry.second->getID(),
                                                        entry.second->getAppName(),
                                                        entry.second->getIconPath(),
                                                        entry.second->supportsScaling(),
                                                        entry.second->canHaveMultipleFrontends(),
                                                        false,
                                                        this));
        }
        for (const auto &entry : appload::library::getExternals()) {
            _applications.append(new AppLoadApplication(entry.first,
                                                        entry.second->getAppName(),
                                                        entry.second->getIconPath(),
                                                        false,
                                                        false,
                                                        true,
                                                        this));
        }
    }

signals:
    void applicationsChanged();

private:
    static void appendApplication(QQmlListProperty<AppLoadApplication> *list, AppLoadApplication *app) {
        auto *lib = qobject_cast<AppLoadLibrary *>(list->object);
        if (lib) {
            lib->_applications.append(app);
        }
    }

    static qsizetype applicationCount(QQmlListProperty<AppLoadApplication> *list) {
        auto *lib = qobject_cast<AppLoadLibrary *>(list->object);
        return lib ? lib->_applications.size() : 0;
    }

    static AppLoadApplication *applicationAt(QQmlListProperty<AppLoadApplication> *list, qsizetype index) {
        auto *lib = qobject_cast<AppLoadLibrary *>(list->object);
        return lib ? lib->_applications.at(index) : nullptr;
    }

    static void clearApplications(QQmlListProperty<AppLoadApplication> *list) {
        auto *lib = qobject_cast<AppLoadLibrary *>(list->object);
        if (lib) {
            qDeleteAll(lib->_applications);
            lib->_applications.clear();
        }
    }

    void clearApplications() {
        qDeleteAll(_applications);
        _applications.clear();
    }

    QList<AppLoadApplication *> _applications;
};
