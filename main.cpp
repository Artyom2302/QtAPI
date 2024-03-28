#include <QCoreApplication>
#include <QtHttpServer>

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

QJsonDocument getDoc(const QString& fileName){
    QByteArray byteArray = getFileBytes(fileName);
    QJsonDocument jsonDoc = QJsonDocument::fromJson(byteArray);
    return jsonDoc;
}


bool LoadFile(const QString& fileName,QJsonObject jsonObject){
    if (QDir::current().exists(fileName)){
        return false;
    }
    QFile file(fileName);
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




void createRoutes(QHttpServer& server,QString& curScheme){

    server.route("/",QHttpServerRequest::Method::Get,[](){
        return "Welcome to HTTP server";

    });

    server.route("/schema",QHttpServerRequest::Method::Get,[](){
        QStringList fileList = QDir().entryList(QDir::NoDotAndDotDot|QDir::AllEntries);

        if (!fileList.isEmpty()) {
            QJsonObject jsonObject;
            QJsonArray jsonArray;

            for (const QString& str : fileList) {
                jsonArray.append(str);
            }
            jsonObject["schemas"] = jsonArray;
            QJsonDocument jsonDocument(jsonObject);
            return QHttpServerResponse(jsonDocument.toJson());
        }
        else {
            return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
        }
    });


    server.route("/schema/<arg>",QHttpServerRequest::Method::Get,[&curScheme](QString name){
        QStringList fileList = QDir().entryList();
        if (fileList.contains(name)) {
            qDebug()<<name;
            curScheme = name;
            qDebug()<<curScheme;
            auto jsonDoc = getDoc(name);
            return QHttpServerResponse(jsonDoc.object());
        }
        else {
            return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
        }
    });

    server.route("/schema/<arg>",QHttpServerRequest::Method::Post,[](QString name,const QHttpServerRequest& request){
        QJsonObject json = QJsonDocument::fromJson(request.body()).object();
        bool load = LoadFile(name+".json",json);
        return  QHttpServerResponse(load ? QHttpServerResponder::StatusCode::Ok : QHttpServerResponder::StatusCode::BadRequest );
    });
    server.route("/object",QHttpServerRequest::Method::Get,[&curScheme](){
        QJsonDocument jsonDoc = getDoc(curScheme);
        QJsonObject jsonObject = jsonDoc.object();
        return QHttpServerResponse(jsonObject["objects"].toArray(),QHttpServerResponder::StatusCode::Ok);
    });

    server.route("/object/<arg>",QHttpServerRequest::Method::Get,[&curScheme](int id){
        QJsonDocument jsonDoc = getDoc(curScheme);
        QJsonObject jsonObject = jsonDoc.object();
        QJsonArray objectsArray = jsonObject["objects"].toArray();

        for (const auto& obj : objectsArray) {

            QJsonObject respObj = obj.toObject();

            if (respObj["id"].toInt() == id) {

                return QHttpServerResponse(respObj);

            }
        }
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
    });
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QHttpServer server;
    QString curScheme = "";
    pathConfig();
    createRoutes(server,curScheme);
    const auto port = 8080;
    auto selectedPort = server.listen(QHostAddress::LocalHost,port);
    qDebug() << "Local server start on "<< selectedPort;
    return a.exec();
}
