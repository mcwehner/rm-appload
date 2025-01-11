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

class AppLoad : public QObject
{
    Q_PROPERTY(QString applicationID READ applicationID WRITE setApplicationID)
    Q_OBJECT
public:
    ~AppLoad();

    QString applicationID() const;
    void setApplicationID(const QString &app);
    Q_INVOKABLE void sendMessage(int type, const QString &string);
    Q_INVOKABLE void terminate();
// Internal C++ API:
    void propagateMessage(int type, const QString &string);

signals:
    void messageReceived(int type, QString contents);
private:
    QString _applicationID;
};

