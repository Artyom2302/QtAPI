#include <QCoreApplication>
#include <QtHttpServer>

 //Настройка текущего каталога
void pathConfig(){
    QDir currentDir = QDir::current();
    currentDir.cdUp();
    qDebug() << currentDir.path();
    if (currentDir.cd("SimpleAPI")){
        if (currentDir.cd("json")){
            QDir::setCurrent(currentDir.path());
        }
    };
    qDebug()<<QDir().currentPath();
}
QByteArray getFileBytes(const QString& fileName){
    QString filePath = QDir::current().filePath(fileName);
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open file:" << file.errorString();
        return QByteArray();
    }

    QByteArray fileContent = file.readAll();
    file.close();
    return fileContent;
}
bool LoadFile(const QString& fileName,QJsonObject jsonObject){
    if (QDir::current().exists(fileName)){
        return false;
    }
    QFile file(fileName+".json");
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QJsonDocument jsonDocument(jsonObject);
    QByteArray jsonData = jsonDocument.toJson();

    qint64 bytesWritten = file.write(jsonData);
    file.close();

    if (bytesWritten != jsonData.size()) {
        return false;
    }

    return true;
}



void createRoutes(QHttpServer& server){
    server.route("/",QHttpServerRequest::Method::Get,[](){
        return "Welcome to HTTP server";

    });
    server.route("/schema",QHttpServerRequest::Method::Get,[](const QHttpServerRequest& request){
        QStringList fileList = QDir().entryList();
        QString schemaName = request.query().queryItemValue("schemaName");
        if (fileList.contains(schemaName)) {

            QByteArray byteArray = getFileBytes(schemaName);
            QJsonDocument jsonDoc = QJsonDocument::fromJson(byteArray);
            QJsonObject json = jsonDoc.object();


            return QHttpServerResponse(json);
        }
        else {
            return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
        }
    });
    server.route("/schema",QHttpServerRequest::Method::Post,[](const QHttpServerRequest& request){
        QJsonObject json = QJsonDocument::fromJson(request.body()).object();
        bool load = LoadFile(request.query().queryItemValue("name"),json);
        return  QHttpServerResponse(load ? QHttpServerResponder::StatusCode::Ok : QHttpServerResponder::StatusCode::BadRequest );
    });
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QHttpServer server;
    pathConfig();
    createRoutes(server);
    const auto port = 8080;
    auto selectedPort = server.listen(QHostAddress::LocalHost,port);
    qDebug() << "Local server start";
    return a.exec();
}
