#include <QFile>
#include <QStack>
#include <iostream>
#include "backend.h"

BackendApi::BackendApi(QHash<QString, QPair<QString, QString>> const &database): database{database} {}

QString BackendApi::getFunctionCode(QString usr)
{
    auto d_it = database.find(usr);
    std::cout << d_it.value().second.toStdString() << "\n\n";
    return d_it.value().second;
}

QString BackendApi::getFunctionName(QString usr)
{
    auto d_it = database.find(usr);
    return d_it.value().first;
}

