#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>

bool startsWithVowel(const QString& str) {
    return str.startsWith('A', Qt::CaseInsensitive) || str.startsWith('E', Qt::CaseInsensitive) ||
           str.startsWith('I', Qt::CaseInsensitive) || str.startsWith('O', Qt::CaseInsensitive) ||
           str.startsWith('U', Qt::CaseInsensitive);
}

QString processCSVFile(const QString& fileData, int& replacements, int& deletions) {
    QString processedData;
    QString tempData = fileData;
    QTextStream in(&tempData);
    QStringList lines = tempData.split('\n');

    for (QString line : lines) {
        QStringList fields = line.split(',');

        bool shouldDelete = false;
        for (const QString& field : fields) {
            if (startsWithVowel(field)) {
                shouldDelete = true;
                break;
            }
        }

        if (shouldDelete) {
            deletions++;
            continue;
        }

        QStringList modifiedFields;
        for (QString field : fields) {
            for (int i = 0; i < field.size(); ++i) {
                if (field[i].isDigit() && (field[i].digitValue() % 2 == 1)) {
                    field[i] = '#';
                    replacements++;
                }
            }
            modifiedFields.append(field);
        }

        processedData.append(modifiedFields.join(',') + "\n");
    }

    return processedData;
}

void handleClient(QTcpSocket* clientSocket) {
    if (clientSocket->waitForReadyRead()) {

        QByteArray requestData = clientSocket->readAll();

        QJsonDocument jsonRequest = QJsonDocument::fromJson(requestData);
        QJsonObject jsonObj = jsonRequest.object();

        QString fileData = jsonObj["filedata"].toString();

        int replacements = 0, deletions = 0;
        QString processedFileData = processCSVFile(fileData, replacements, deletions);

        QJsonObject response;
        response["filedata"] = QString(processedFileData.toUtf8().toBase64());
        response["replacements"] = replacements;
        response["deletions"] = deletions;

        clientSocket->write(QJsonDocument(response).toJson());
        clientSocket->flush();
    }

    clientSocket->disconnectFromHost();
    clientSocket->deleteLater();
}

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    if (argc < 2) {
        qDebug() << "Usage:<port>";
        return -1;
    }
    QString serverPort = argv[1];
    quint16 port = serverPort.toUShort();
    QTcpServer server;

    if (!server.listen(QHostAddress::Any, port)) {
        qDebug() << "Server failed to start!";
        return -1;
    }

    qDebug() << "Server started on port"<<serverPort;

    while (true) {
        if (server.waitForNewConnection(-1)) {
            QTcpSocket* clientSocket = server.nextPendingConnection();
            handleClient(clientSocket);
        }
    }

    return a.exec();
}
