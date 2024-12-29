#pragma once
#include <map>
#include <vector>
#include <thread>

#include "AppLoad.h"
#include "protocol.h"
#include "AppLoadCoordinator.h"

#define SOCKET_BACKLOG 10
#define RESP_ERR 1
#define RESP_OK 0

namespace appload::management {
    static std::map<QString, std::vector<AppLoad*>> endpoints;
    static std::vector<AppLoadCoordinator*> coordinators;
    static std::map<QString, std::pair<int, int>> sockets;

    void registerEndpoint(QString id, AppLoad *endpoint);
    void unregisterEndpoint(QString id, AppLoad *endpoint);

    void registerCoordinator(AppLoadCoordinator *coordinator);
    void unregisterCoordinator(AppLoadCoordinator *coordinator);

    void sendMessageTo(const QString &id, int messageType, const QString &message);
    void broadcastMessageToControllers(const QString &id, int messageType, const QString &message);

    void _registerSocket(const QString &applicationID, int socket, int pipe);

    void terminate(const QString &id);
    void closeUIInstance(const QString &id, bool force=false);
    void createListeningSocket(QString id, char *socketPath);

    bool isBackendRunningFor(const QString &applicationID);
}
