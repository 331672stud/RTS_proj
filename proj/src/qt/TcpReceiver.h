#pragma once
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

class TcpReceiver : public QObject
{
    Q_OBJECT
public:
    explicit TcpReceiver(QObject *parent = nullptr);

public slots:
    void startServer(int port);
    void stopServer();

signals:
    void waypointsJson(const QString &json);
    void graphUpdate(uint64_t u, uint64_t v, double newWeight);
    void stopped();   // emitted when cleanup is finished

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    QTcpServer *m_server = nullptr;
    QTcpSocket *m_clientSocket = nullptr;
    std::string m_remaining;    // buffer for partial lines
};