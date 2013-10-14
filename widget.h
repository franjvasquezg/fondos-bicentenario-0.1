/*
        This Program was written by Eric Baudach <Eric.Baudach@web.de>
        and is licensed under the GPL version 3 or newer versions of GPL.

                    <Copyright (C) 2010-2011 Eric Baudach>
 */

#ifndef WIDGET_H
#define WIDGET_H

#include <glib.h>
#include "ui_widget.h"
#include <QtGui>
#include "threadb.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>


namespace Ui {
class Widget;

}
class MyException : public QtConcurrent::Exception
{
public:
    void raise() const {
	throw *this;
    }
    Exception *clone() const {
        return new MyException(*this);
    }
};
class Widget : public QWidget
{
    Q_OBJECT
public:
    Widget(QWidget *parent = 0);
    ~Widget();
    QSettings *Settings;
    QtMsgHandler MessageHandler;
    QStringList checkstatelist;
    QStringList stringlist;

protected:
    void changeEvent(QEvent *e);

private:
    void setTooltipText();
    void adjustSizer();
    void CheckVersion();
    bool WidgetIcon;
    Ui::Widget *ui;
    bool ImageSavedWatcher;
    int current;
    int firstload;
    void adjustPosition(QWidget*);
    QFileSystemWatcher *watcher;
    void setWP(QString);
    QStringList ItemList;
    int Counter;
    QStringList waitOnShow;
    void LoadSettings();
    void SaveSettings();
    void CreateUI();
    void DateiListe();
    QStringList DateiListeNeu();
    int Fehler;
    QTimer *intervalTimer;
    QStringList Files;
    QFileDialog *FD;
    QSystemTrayIcon *Tray;
    QString currentWP();
    QString WPDir;
    threadb *b;
    bool firstTime;
    QList<QLabel*> SliderLabel;
    QMutex mutex;
    QString Ja;
    QString Nein;
    QMenu *TrayContextMenu;
    QAction *removeAction;
    void statisticer();
    void LoopWatcher();
    QTime Started;
    void loadDirs();
    void readDates();
    QStringList  datesList;


Q_SIGNALS:
    void WPDirPath(QString);
    void toAddSignal(const QStringList &);
    void toRemSignal(const QStringList &);
    void shown(const QStringList &);
    void ThreadShouldSleep(int);


private Q_SLOTS:
    void DirChanged();
    void toRem(const QStringList&);
    void itemCheckStateChanged(QListWidgetItem*);
    void downloadFinished(QNetworkReply*);
    void counterFinished(QNetworkReply*);
    void StartGui();
    void showEfeMsg();

public Q_SLOTS:
    void about();
    void addToList(const QImage&,const QString&);
    void listWidget_doubleClicked(QModelIndex);
    void RandomWallpaper();
    void DirDialog();
    void FDChangeDir(int);
    void TrayClick(QSystemTrayIcon::ActivationReason);
    void hider();
    void DirListRem();
    void ThreadShouldStop();
    void ImageSaved(bool);


private Q_SLOTS:
    void SaveWpStyle(int);
    void setAutoStart(bool);
    void DeleteCurrentWP();
    void DeleteSelectedWP();
    void listWidget_customContextMenuRequested(const QPoint &);
    void YesDeleteSelectedWP(QAbstractButton*);
    void YesDeleteCurrentWP(QAbstractButton*);
    void on_timeEdit_timeChanged(QTime date);
};

#endif // WIDGET_H
