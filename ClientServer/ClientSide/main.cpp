#include <QCoreApplication>
#include <QTcpSocket>
#include <QFile>
#include <QTextStream>
#include <QRandomGenerator>
#include <QJsonDocument>
#include <QJsonObject>

QString generateRandomString() {
    QString possibleCharacters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    QString randomString;
    for (int i = 0; i < 8; ++i)
        randomString.append(possibleCharacters.at(QRandomGenerator::global()->bounded(possibleCharacters.length())));
    return randomString;
}

void generateCSVFile(const QString& fileName) {
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly  | QIODevice::Text)) {
        QTextStream stream(&file);
        for (int i = 0; i < 1024; ++i) {
            for (int j = 0; j < 6; ++j) {
                stream << generateRandomString();
                if (j != 5)
                    stream << ",";
            }
            stream << "\n";
        }
        file.close();
    }
}

void sendFileToServer(QTcpSocket& socket, const QString& fileName) {
    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray fileData = file.readAll();
        QJsonObject request;
        request["filedata"] = QString(fileData);
        QJsonDocument jsonDoc(request);

        QByteArray jsonData = jsonDoc.toJson();

        socket.write(jsonData);
        socket.flush();
        file.close();
    }
}

void receiveFileFromServer(QTcpSocket& socket, const QString& outputFile) {
    if (socket.waitForReadyRead()) {
        QByteArray responseData = socket.readAll();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(responseData);
        QJsonObject jsonObj = jsonResponse.object();

        QString modifiedFileData = QByteArray::fromBase64(jsonObj["filedata"].toString().toUtf8());
        QFile file(outputFile);
        if (file.open(QIODevice::WriteOnly  | QIODevice::Text)) {
            file.write(modifiedFileData.toUtf8());
            file.close();
        }

        int replacements = jsonObj["replacements"].toInt();
        int deletions = jsonObj["deletions"].toInt();
        qDebug() << "Replacements: " << replacements;
        qDebug() << "Deletions: " << deletions;
    }
}
int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    if (argc < 5) {
        qDebug() << "Usage: client <input_file> <output_file> <server_address> <port>";
        return -1;
    }

    QString inputFile = argv[1];
    QString outputFile = argv[2];
    QString serverAddress = argv[3];
    QString serverPort = argv[4];
    quint16 port = serverPort.toUShort();


    generateCSVFile(inputFile);

    QTcpSocket socket;
    socket.connectToHost(serverAddress, port);

    if (socket.waitForConnected()) {
        sendFileToServer(socket, inputFile);
        receiveFileFromServer(socket, outputFile);
        socket.disconnectFromHost();
        qDebug() << "The End";
    } else {
        qDebug() << "Failed to connect to server!";
    }

    return a.exec();
}


