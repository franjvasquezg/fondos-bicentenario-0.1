#ifndef HELPER_H
#define HELPER_H

#include <QObject>
#include <QRunnable>
#include <QDebug>
#include <QThread>
#include <QString>
#include <QWidget>
#include <QStringList>
#include <QString>
#include <QDir>
#include <QFile>
#include <QMetaType>


class helper : public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit helper();
    virtual void run();
    QString recursiv;
    QString path;

};


#endif // HELPER_H
