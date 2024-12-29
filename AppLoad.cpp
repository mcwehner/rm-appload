#include "AppLoad.h"
#include "management.h"

void AppLoad::setApplicationID(const QString &app) {
    if(!_applicationID.isEmpty()) {
        appload::management::unregisterEndpoint(_applicationID, this);
    }
    _applicationID = app;
    appload::management::registerEndpoint(app, this);
}

AppLoad::~AppLoad() {
    appload::management::unregisterEndpoint(_applicationID, this);
}

QString AppLoad::applicationID() const {
    return _applicationID;
}

void AppLoad::propagateMessage(int type, const QString &string){
    emit messageReceived(type, string);
}

void AppLoad::sendMessage(int type, const QString &message){
    appload::management::sendMessageTo(_applicationID, type, message);
}

void AppLoad::quit(){
    appload::management::terminate(_applicationID);
}
