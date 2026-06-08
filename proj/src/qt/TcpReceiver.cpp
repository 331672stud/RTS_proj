#include "TcpReceiver.h"
#include <simdjson.h>
#include <QDebug>

TcpReceiver::TcpReceiver(QObject *parent) : QObject(parent) {}

void TcpReceiver::startServer(int port)
{
    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection,
            this, &TcpReceiver::onNewConnection);
    if (!m_server->listen(QHostAddress::Any, port)) {
        qWarning() << "Server could not start:" << m_server->errorString();
        return;
    }
    qDebug() << "[TCP] Listening on port" << port;
}

void TcpReceiver::stopServer()
{
    // 1. Stop accepting new connections
    if (m_server) {
        m_server->close();
        m_server->deleteLater();
        m_server = nullptr;
    }
    // 2. Disconnect any active client
    if (m_clientSocket) {
        m_clientSocket->disconnectFromHost();
        // m_clientSocket will be deleted via onDisconnected or deleteLater
    }
    // 3. After everything is closed, schedule our own deletion
    //    and tell the outside world that we are done.
    deleteLater();               // will be executed when the event loop runs
    emit stopped();
}

void TcpReceiver::onNewConnection()
{
    // Only one client – reject any new one if already connected
    if (m_clientSocket) {
        QTcpSocket *rejected = m_server->nextPendingConnection();
        rejected->close();
        rejected->deleteLater();
        return;
    }
    m_clientSocket = m_server->nextPendingConnection();
    connect(m_clientSocket, &QTcpSocket::readyRead,
            this, &TcpReceiver::onReadyRead);
    connect(m_clientSocket, &QTcpSocket::disconnected,
            this, &TcpReceiver::onDisconnected);
    qDebug() << "[TCP] Client connected";
}

void TcpReceiver::onReadyRead()
{
    // Accumulate data and parse complete lines
    m_remaining.append(m_clientSocket->readAll().toStdString());

    simdjson::dom::parser parser;
    size_t pos;
    while ((pos = m_remaining.find('\n')) != std::string::npos) {
        std::string line = m_remaining.substr(0, pos);
        m_remaining.erase(0, pos + 1);
        if (line.empty()) continue;

        simdjson::dom::element doc;
        if (parser.parse(line).get(doc)) continue;

        std::string_view type;
        if (doc["type"].get_string().get(type)) continue;

        if (type == "waypoints") {
            emit waypointsJson(QString::fromStdString(line));
        }
        else if (type == "graph_update") {
            simdjson::dom::array edge;
            uint64_t u, v;
            double w;
            if (doc["edge"].get_array().get(edge) ||
                edge.at(0).get_uint64().get(u) ||
                edge.at(1).get_uint64().get(v) ||
                doc["new_weight"].get_double().get(w)) continue;
            emit graphUpdate(u, v, w);
        }
    }
}

void TcpReceiver::onDisconnected()
{
    qDebug() << "[TCP] Client disconnected";
    m_clientSocket->deleteLater();
    m_clientSocket = nullptr;
}