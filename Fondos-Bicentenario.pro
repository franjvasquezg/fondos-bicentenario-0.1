# -------------------------------------------------
# Project created by QtCreator 2010-05-20T11:56:49
# -------------------------------------------------
TARGET = fondos-bicentenario
TEMPLATE = app
SOURCES += main.cpp \
    threadb.cpp \
    widget.cpp \
    helper.cpp
HEADERS += widget.h \
    threadb.h \
    helper.h
FORMS += widget.ui
CONFIG += link_pkgconfig \
    qt \
release
PKGCONFIG += gconf-2.0 \
glib-2.0 \
gobject-2.0
PKGCONFIG -= ORBit-2.0 \
dbus-1
QT += network
TRANSLATIONS += po/ca.ts \
po/cs.ts \
po/de.ts \
po/es.ts \
po/fa.ts \
po/fi.ts \
po/fr.ts \
po/ia.ts \
po/it.ts \
po/kk.ts \
po/ko.ts \
po/nl.ts \
po/pl.ts \
po/pt_BR.ts \
po/pt.ts \
po/ru.ts \
po/si.ts \
po/sv.ts \
po/th.ts \
po/tr.ts \
po/uk.ts \
po/zh_CN.ts
background.path = /usr/share/fondos-bicentenario/
background.files += backgrounds/
background.files += res/
INSTALLS += background
icon.path = /usr/share/pixmaps
icon.files += fondos-bicentenario.png \
fondos-bicentenario2.png
INSTALLS += icon
target.path = /usr/bin
INSTALLS += target
desktop.path = /usr/share/applications
desktop.files = fondos-bicentenario.desktop
INSTALLS += desktop
languages.path = /usr/share/qt4/translations/fondos-bicentenario
languages.extra = $(QTDIR)lrelease -silent Fondos-Bicentenario.pro
INSTALLS += languages
translations.path = /usr/share/qt4/translations/fondos-bicentenario
translations.files = po/*.qm
INSTALLS += translations
RESOURCES += \
    rc.qrc
CODECFORSRC = UTF-8
