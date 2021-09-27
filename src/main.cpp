#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QFile>
#include <unordered_map>
#include "backend.h"
#include "indexer.h"

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    const QUrl url(QStringLiteral("qrc:/Main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    QHash<QString, QPair<QString, QString>> database;
    std::unordered_map<std::string, std::pair<std::string, std::string>> hmap;

    run_indexer(hmap);

    for (auto &[k, v] : hmap) {
        database.insert(QString::fromStdString(k), qMakePair(QString::fromStdString(v.first), QString::fromStdString(v.second)));
    }

    qmlRegisterSingletonType<BackendApi>("CodeViewer", 1, 0, "BackendApi", [&database](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject * {
    	Q_UNUSED(engine)
    	Q_UNUSED(scriptEngine)
        
        return new BackendApi(database);
    });

    engine.load(url);

    return app.exec();
}
