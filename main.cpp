/*
        This Program was written by Eric Baudach <Eric.Baudach@web.de>
        and is licensed under the GPL version 3 or newer versions of GPL.

                    <Copyright (C) 2010-2011 Eric Baudach>
 */

#include <QtGui/QApplication>
#include <widget.h>
#include <QTranslator>
#include <QGtkStyle>



int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //a.setGraphicsSystem("opengl");
    a.setEffectEnabled(Qt::UI_AnimateCombo, true);
    a.setEffectEnabled(Qt::UI_AnimateMenu, true);
    a.setEffectEnabled(Qt::UI_FadeTooltip, true);
    a.setEffectEnabled(Qt::UI_FadeMenu, true);
    a.setStyle(QStyleFactory::create("GTK+"));
    
    QTextCodec *linuxCodec = QTextCodec::codecForName("UTF-8");
    QTextCodec::setCodecForTr(linuxCodec);
    QTextCodec::setCodecForCStrings(linuxCodec);
    QTextCodec::setCodecForLocale(linuxCodec);

    QTranslator qtTranslator;
    if(!qtTranslator.load(QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath) + "/fondos-bicentenario")) {
        if(QLocale::system().name().startsWith("en_") == false) {
            qDebug() << "Can't load translation file:";
            qDebug() << QLocale::system().name() + ".qm in " + QLibraryInfo::location(QLibraryInfo::TranslationsPath) + "/fondos-bicentenario";
        }
    }
    a.installTranslator(&qtTranslator);
    a.setApplicationName("Fondos de Pantalla del Bicentenario");
    a.setApplicationVersion("0.1");
    Widget w;
    return a.exec();
}
