//
// Created by Frank Steiler on 9/28/16.
// Copyright (c) 2016 Hewlett-Packard. All rights reserved.
//
// Utility_Json.cpp: [...]
//
#include "Utility.h"
#include <qjson/parser.h>
#include <qjson/serializer.h>
#include <QDebug>
#include <QFile>

QVariant Utility::Json::parse(const QByteArray &json) {
    LDEBUG << "Parsing JSON: " << json.constData();
    QJson::Parser parser;
    bool ok;
    QVariant result = parser.parse(json, &ok);

    if (!ok) {
        LFATAL << "Error parsing json";
        result.clear();
    }

    return result;
}

QVariant Utility::Json::loadFromFile(const QString &filename) {
    LDEBUG << "Loading JSON from file " << filename.toUtf8().constData();
    QVariant result;
    QFile f(filename);
    QJson::Parser parser;
    bool ok;

    if (!f.open(f.ReadOnly)) {
        LFATAL << "Error opening file:" << filename.toUtf8().constData();
        return result;
    }

    result = parser.parse(&f, &ok);
    f.close();

    if (!ok) {
        LFATAL << "Error parsing json file:" << filename.toUtf8().constData();
        result.clear();
    }

    return result;
}

bool Utility::Json::saveToFile(const QString &filename, const QVariant &json) {
    LDEBUG << "Saving JSON to file " << filename.toUtf8().constData();

    QFile f(filename);
    QJson::Serializer serializer;
    bool ok;

    if (!f.open(f.WriteOnly)) {
        LFATAL << "Error opening file for writing: " << filename.toUtf8().constData();
        return false;
    }
    serializer.setIndentMode(QJson::IndentFull);
    serializer.serialize(json, &f, &ok);
    f.close();

    if (!ok) {
        LFATAL << "Error serializing json to file:" << filename.toUtf8().constData();
        return false;
    } else {
        return true;
    }
}

