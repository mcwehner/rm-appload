#pragma once

#include <map>
#include <string>
#include <mutex>
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include "management.h"

#include <QByteArray>
#include <QResource>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "config.h"

namespace appload::library {
    class ExternalApplication {
        public:
        ExternalApplication(QString root);
        QString getIconPath() const;
        QString getAppName() const;
        void launch() const;

        bool valid = false;

        private:
        QString root;

        QString appName;
        QString execPath;

        QString workingDirectory;
        QStringList args;
        std::map<QString, QString> environment;

        void parseManifest();
    };

    class LoadedApplication {
    public:
        LoadedApplication(QString root);
        ~LoadedApplication();
        void startAppBackend();
        void load();
        void loadFrontend();
        void unloadFrontend();

        QString getIconPath() const;
        QString getAppName() const;
        QString getID() const;
        QString getFrontendRoot() const;
        QString getQMLEntrypoint() const;
        bool isBackendRequired() const;
        bool isFrontendRunning() const;
        bool supportsScaling() const;
        bool canHaveMultipleFrontends() const;
        bool valid = false;
        bool currentlyUnloading = false;
        int loadedFrontendInstanceCount = 0;
    private:
        QString root;
        QString appName, appID, qmlEntrypoint;
        QString frontendRoot;
        QString internalIdentifier;
        bool loadsBackend;
        bool _supportsScaling;
        bool _canHaveMultipleFrontends;
        bool frontendLoaded = false;
        void parseManifest();
    };

    static std::map<QString, LoadedApplication*> applications;
    static std::map<QString, ExternalApplication*> externalApplications;
    int loadApplications();
    appload::library::LoadedApplication *get(const QString &id);
    const std::map<QString, appload::library::LoadedApplication*> &getRef();
    const std::map<QString, appload::library::ExternalApplication*> &getExternals();
};
