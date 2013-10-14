/*
        This Program was written by Eric Baudach <Eric.Baudach@web.de>
        and is licensed under the GPL version 3 or newer versions of GPL.

                    <Copyright (C) 2010-2011 Eric Baudach>
 */

#include "helper.h"
#include <QProcess>

helper::helper() : QRunnable()
{
}

void helper::run()
{
    //QProcess exec;
    if(recursiv == "2"){
        QDir verzeichnis = QDir(path);
        QStringList filters;
        const QStringList & SubDirs = verzeichnis.entryList(QDir::Dirs | QDir::Hidden, QDir::NoSort);
        filters << "*.thumb_*";
        verzeichnis.setNameFilters(filters);
        Q_FOREACH(QString Datei,verzeichnis.entryList(verzeichnis.nameFilters(), QDir::Files | QDir::Hidden | QDir::Writable, QDir::NoSort)) {
            QFile::remove(verzeichnis.path() + "/" + Datei);
            //qDebug() << path + "/" + Datei;
        }

        Q_FOREACH(QString dir, SubDirs) {
            verzeichnis.setPath(path + "/" + dir);
            Q_FOREACH(QString Datei,verzeichnis.entryList(verzeichnis.nameFilters(), QDir::Files | QDir::Hidden | QDir::Writable, QDir::NoSort)) {
                QFile::remove(verzeichnis.path() + "/" + Datei);
                //qDebug() << verzeichnis.path() + "/" + Datei;
            }
        }

    }else{
        QDir verzeichnis = QDir(path);
        QStringList filters;
        filters << "*.thumb_*";
        verzeichnis.setNameFilters(filters);

        Q_FOREACH(QString Datei,verzeichnis.entryList(verzeichnis.nameFilters(), QDir::Files | QDir::Hidden | QDir::Writable, QDir::NoSort)) {
            QFile::remove(path + "/" + Datei);
            //qDebug() << path + "/" + Datei + " oder " + verzeichnis.path() + "/" + Datei;

        }
    }
    return;
}

