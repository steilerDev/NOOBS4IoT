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

QMap<QString, QVariant> Utility::Json::parseJson(const QString &jsonString) {
    return Utility::Json::parseJson(jsonString.toUtf8());
};

QMap<QString, QVariant> Utility::Json::parseJson(const QByteArray &jsonArray) {
    LDEBUG << "Parsing JSON: " << jsonArray.constData();
    QJson::Parser parser;
    bool ok;
    QMap<QString, QVariant> json = parser.parse(jsonArray, &ok).toMap();
    if(!ok) {
        LERROR << "Unable to parse JSON";
        json.clear();
    } else {
        LDEBUG << "Succesfully parsed JSON";
    }

    return json;
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

void Utility::Json::printJson(QMap<QString, QVariant> &json) {
    LDEBUG << "{";
            foreach(QString key, json.keys()) {
            // We have a simple key:value
            if(json[key].canConvert<QString>()) {
                LDEBUG << key.toUtf8().constData() << ": " << json[key].toString().toUtf8().constData();

                // We have key: [ ... ]
            } else if (json[key].canConvert<QList<QVariant> >()) {
                LDEBUG << key.toUtf8().constData() << ":";
                QList<QVariant> embeddedArray = json[key].toList();
                printJsonArray(embeddedArray);
                // We have key : { ... }
            } else if (json[key].canConvert<QMap<QString, QVariant> >()) {
                LDEBUG << key.toUtf8().constData() << ":";
                QMap<QString, QVariant> embeddedJson = json[key].toMap();
                printJson(embeddedJson);
            } else {
                LERROR << "Unable to parse part of this";
            }
        }
    LDEBUG << "}";
}

void Utility::Json::printJsonArray(QList<QVariant> &json) {
    LDEBUG << "[";
            foreach(QVariant listItem, json) {
            // We have a string
            if(listItem.canConvert<QString>()) {
                LDEBUG << listItem.toString().toUtf8().constData();
                // We have an embedded JSON
            } else if(listItem.canConvert<QMap<QString, QVariant> >()) {
                QMap<QString, QVariant> embeddedJson = listItem.toMap();
                printJson(embeddedJson);
                // We have an embedded array
            } else if (listItem.canConvert<QList<QVariant> >()) {
                QList<QVariant> embeddedArray = listItem.toList();
                printJsonArray(embeddedArray);
            } else {
                LERROR << "Unable to parse part of this";
            }
        }
    LDEBUG << "]";
}

