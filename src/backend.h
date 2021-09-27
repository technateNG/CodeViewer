#pragma once

#include <QObject>
#include <QHash>

class BackendApi : public QObject
{
    Q_OBJECT

    QHash<QString, QPair<QString, QString>> const &database;

public:
    explicit BackendApi(QHash<QString, QPair<QString, QString>> const &database);

    ~BackendApi() override = default;

    Q_INVOKABLE QString getFunctionCode(QString usr);

    Q_INVOKABLE QString getFunctionName(QString usr);
};
