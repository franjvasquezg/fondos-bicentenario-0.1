/*
        This Program was written by Eric Baudach <Eric.Baudach@web.de>
        and is licensed under the GPL version 3 or newer versions of GPL.

                    <Copyright (C) 2010-2011 Eric Baudach>
 */

#include "widget.h"
#include "ui_widget.h"
#include "helper.h"
#include <gconf/2/gconf/gconf-client.h>
#include <QNetworkProxyFactory>


/**
 * Maneja la salida de mensajes
 * @param type
 * @param msg
 */
void MessageOutput(QtMsgType type, const char *msg)
{
    QString msg2;
    switch (type) {
      
      //Mensajes de debug
      case QtDebugMsg:
          //fprintf(stderr, "Debug: %s\n", msg);
          break;
          
      //Mensajes de advertencia
      case QtWarningMsg:
          if(QString(msg).contains("null image")) {
              qDebug() << "Couldn't load image!";
              return;
          }
          if(QString(msg).contains("Application asked to unregister timer")) {
              qDebug() << "Application asked to unregister timer, which is not registered in this thread.";
              return;
          }
          fprintf(stderr, "Warning: %s\n", msg);
          //QMessageBox::warning(NULL, "Warning", QString(msg).remove('"'));
          break;
          
      //Mensaje Error crítico    
      case QtCriticalMsg:
          fprintf(stderr, "Critical: %s\n", msg);
          msg2 = QString(msg) % "\n\nPlease report the bug at:\nhttp://bicentenario.cenditel.gob.ve";
          QMessageBox::critical(NULL, "Critical", QString(msg2).remove('"'));
          break;
      //Mensaje error fatal    
      case QtFatalMsg:
          fprintf(stderr, "Fatal: %s\n", msg);
          msg2 = QString(msg) % "\n\nPlease report the bug at:\nhttp://bicentenario.cenditel.gob.ve";
          QMessageBox::critical(NULL, "Fatal", QString(msg2).remove('"'));
          abort();
          break;
    }
}

/**
 * 
 * @param parent
 */
Widget::Widget(QWidget *parent) :
        QWidget(parent),
        ui(new Ui::Widget)
{    
    MessageHandler = qInstallMsgHandler(MessageOutput);
    ui->setupUi(this);
    this->statisticer();
    this->current = 0;
    this->firstload = 0;
    this->firstTime = false;    
    this->WidgetIcon = false;
    this->ImageSavedWatcher = false;


    //Crea el icono de la bandeja del SO
    Tray = new QSystemTrayIcon(QIcon(QString::fromUtf8(":/icons/fondos-bicentenario.png")), this);
    
    //Crea menu del icono
    TrayContextMenu = new QMenu();
    //Añade la acción Open GUI y la enlaza al metodo StartGui
    TrayContextMenu->addAction(trUtf8("Open GUI"), this, SLOT(StartGui()), 0);
    
    //Eliminar imagen -> lo comentamos por ahora
    
//    const QString removeFromDiskString = QString(trUtf8("remove from disk"));
//    removeAction = new QAction(removeFromDiskString, this);
//    connect(removeAction, SIGNAL(triggered()), this, SLOT(DeleteCurrentWP()));
//    QMenu *CurrentWallpaper = new QMenu(trUtf8("Current wallpaper"));
//    CurrentWallpaper->addAction(removeAction);
//
//    if(!QFileInfo(this->currentWP()).isWritable()) {
//        removeAction->setEnabled(false);
//    }
//    TrayContextMenu->addMenu(CurrentWallpaper);
    
    
    TrayContextMenu->addSeparator(); //Separador
    
    //Añade al menu opción de salir que llama al metodo close
    TrayContextMenu->addAction(trUtf8("Exit"), this, SLOT(close()), 0);
    
    // Enlaza icono con menu
    this->Tray->setContextMenu(TrayContextMenu);

    setTooltipText(); // Invoca metodo que activa tooltips del icono en la bandeja
    
    Tray->show(); // Muestra el icono

    // Conecta click izq con actualización de tiempo de duración de fondo
    // y selección y cambio aleatorio de fondo
    if(!connect(this->Tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(TrayClick(QSystemTrayIcon::ActivationReason)))) {
        qDebug() << "Tray Connect Failure!";
    }
    
    
    // Conecta lista de directorios de la configuración con el metodo
    // itemCheckStateChanged
    connect(ui->listWidget_Dirs, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(itemCheckStateChanged(QListWidgetItem*)));
    
    
    connect(ui->comboBox_wpstyle, SIGNAL(currentIndexChanged(int)), this, SLOT(SaveWpStyle(int)));
    Settings = new QSettings(QSettings::NativeFormat,QSettings::UserScope,QString("Fondos-Bicentenario"),QString("Fondos-Bicentenario"), NULL);
    
    b = new threadb();
    connect(b, SIGNAL(ImageSaved(bool)), this, SLOT(ImageSaved(bool)));
    connect(this, SIGNAL(ThreadShouldSleep(int)), this->b, SLOT(sleeping(int)), Qt::AutoConnection);
    
    this->CreateUI();
    watcher = new QFileSystemWatcher();
    watcher->connect(watcher, SIGNAL(directoryChanged(QString)), this, SLOT(DirChanged()));
    this->LoadSettings();
    this->ui->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->btn_close, SIGNAL(clicked()), this, SLOT(hider()));
    if(!connect(ui->listWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(listWidget_doubleClicked(QModelIndex)))) {
        qDebug() << "listWidget Connect Failure!";
    }
    if(!connect(ui->listWidget, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(listWidget_customContextMenuRequested(const QPoint &)))) {
        qDebug() << "listWidget Connect Failure!";
    }
    if(!connect(ui->checkBox_loadOnStartup, SIGNAL(toggled(bool)), this, SLOT(setAutoStart(bool)))) {
        qDebug() << "checkBox_loadOnStartup Connect Failure!";
    }
    connect(this, SIGNAL(toAddSignal(const QStringList &)), b, SLOT(FileAdded(const QStringList &)), Qt::AutoConnection);
    connect(this, SIGNAL(shown(const QStringList &)), b, SLOT(FileAdded(const QStringList &)));
    connect(this, SIGNAL(toRemSignal(const QStringList &)), this, SLOT(toRem(const QStringList &)));
    connect(this->b, SIGNAL(valueChanged(const QImage &,const QString &)), this, SLOT(addToList(const QImage &,const QString &)), Qt::AutoConnection);
    this->b->start();
    this->DateiListe();
    if(ui->checkBox_changeWPOnStartup->checkState() == Qt::Checked) {
        RandomWallpaper();
    }
    
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(showEfeMsg()));
    timer->start(360000);
    
    Started = QTime::currentTime();
}

  /**
   * Genera tooltips para cada imágen
   */
void Widget::setTooltipText(){
    QString current = this->currentWP();
    QString Pfad = QFileInfo(current).absolutePath();
    QString Datei = QFileInfo(current).fileName();
    QPixmap currentpix = QPixmap(current);
    const QString text = trUtf8("Filename: %1\nPath: %2\nResolution: %3x%4").arg(Datei).arg(Pfad).arg(currentpix.width()).arg(currentpix.height());
    Tray->setToolTip(text);
    //Tray->showMessage(trUtf8("Current wallpaper"), text.arg(Datei).arg(Pfad).arg(currentpix.width()).arg(currentpix.height()), QSystemTrayIcon::NoIcon, 5000);
    currentpix = QPixmap();
    currentpix.detach();

}

void Widget::statisticer(){
    QFile* installed = new QFile(QDir::homePath() % "/.gconf/apps/Fondos-Bicentenario/installed");

    if(installed->exists() == false) {
        QUrl url = QUrl("http://eric32.er.funpic.de/counter.php?version=" % QApplication::applicationVersion());
        QNetworkAccessManager *manager = new QNetworkAccessManager();
        connect(manager, SIGNAL(finished(QNetworkReply*)), SLOT(counterFinished(QNetworkReply*)));
        QNetworkRequest request = QNetworkRequest(url);

        QString var(getenv("http_proxy"));
        QRegExp regex("(http://)?(.*):(\\d*)/?");
        int pos = regex.indexIn(var);

        if(pos > -1){
            QString host = regex.cap(2);
            int port = regex.cap(3).toInt();
            QNetworkProxy proxy(QNetworkProxy::HttpProxy, host, port);
            QNetworkProxy::setApplicationProxy(proxy);
        }

        manager->get(request);
    }else {
        if ( installed->open(QIODevice::ReadOnly) )
        {
            QByteArray installed_version = installed->readAll();
            if(installed_version != QApplication::applicationVersion()){
                installed->close();
                installed->remove();
                this->statisticer();
            }
        }
        else
        {
            QString DebugString = "Could not open file %1";
            qDebug() << DebugString.arg(installed->fileName());
        }
    }
}

void Widget::counterFinished(QNetworkReply* reply){
    QUrl url = reply->url();
    if (!reply->error()) {
        const QString FondosBicentenariogconfdir = QDir::homePath() % "/.gconf/apps/Fondos-Bicentenario";
        QDir *gconfFondosBicentenario = new QDir(FondosBicentenariogconfdir);
        if(!gconfFondosBicentenario->exists()) {
            gconfFondosBicentenario->mkdir(FondosBicentenariogconfdir);
        }
        QFile* installed = new QFile(QDir::homePath() % "/.gconf/apps/Fondos-Bicentenario/installed");
        if ( installed->open(QIODevice::WriteOnly) )
        {

            installed->write(QApplication::applicationVersion().toUtf8());
            installed->close();
        }
        else
        {
            QString DebugString = "Could not create file %1";

            qDebug() << DebugString.arg(installed->fileName());
        }

    }
    
}
    


/**
 * Maneja el estado de chequeo de los directorios de imágenes en la configuración
 * (Modificado completamente por José Ruiz por mal funcionamiento del original)
 * @param item
 */
void Widget::itemCheckStateChanged(QListWidgetItem* item){
        
    if(firstload > 0) { //Si ya fue ejecutado LoadSettings (carga de config)
      
        //Lee lista de directorios 
        QStringList stringlist = Settings->value("listWidget_Dirs").toStringList();         
        //Lee estado de los directorios (activos o inactivos)
        QStringList checkstatelist = Settings->value("listWidget_Dirs_flags").toStringList();
        
        //ui->listWidget_Dirs->setCurrentRow(0, QItemSelectionModel::SelectCurrent);
        
        
        if(stringlist.length() != 0) { //hay items en la lista de directorios
          
            for (int i = 0; i<stringlist.length(); i++) {
                
              //int statuscheckvalue = ui->listWidget_Dirs->selectedItems()[0]->checkState();  
                         
                
                QString statusi = checkstatelist.at(i);
                QString itemi   = stringlist.at(i);
                
                if(item->text() == itemi) { // ubica item modificado
                  
                  if(item->checkState() == 2) { // Si se está chequeando
                    
                    //Busca en toda la lista si hay un directorio que lo incluya y este activo
                    for (int j = 0; j < stringlist.length(); j++) {

                      if(stringlist.at(j) != item->text()){ // si no es el mismo
                        
                        if(stringlist.at(j).startsWith(item->text()) == true and  checkstatelist.at(j).toInt() == 2) { // si es hijo de un directorio activo

                          QString dublicateString = trUtf8("%1 would be double monitored, please remove that entry before!")
                                                  .arg(stringlist.at(j));
                            ui->listWidget_Dirs->selectedItems()[0]->setCheckState(Qt::Unchecked);
                            QMessageBox::information(this, trUtf8("dublicate detected"),  dublicateString);
                            return;
                        }
                        
                        if(item->text().startsWith(stringlist.at(j)) == true and checkstatelist.at(j).toInt() == 2) { // si es hijo de un directorio activo

                          QString dublicateString = trUtf8("%1 would be double monitored, please remove that entry before!")
                                                  .arg(item->text());
                            ui->listWidget_Dirs->selectedItems()[0]->setCheckState(Qt::Unchecked);
                            QMessageBox::information(this, trUtf8("dublicate detected"),  dublicateString);
                            return;
                        }
                      }
                    }
                    
                    checkstatelist.replace(i, "2");                                          
                    
                  } else {
                    
                    checkstatelist.replace(i, "0");
                    
                  }
                  
                }
                
            }
            
              Settings->setValue("listWidget_Dirs_flags", checkstatelist);
              this->DirChanged();
        }
    }
}


void Widget::ThreadShouldStop(){

  this->SaveSettings();
    this->b->terminate();
    delete this->b;
    delete ui;
}

Widget::~Widget()
{
    this->SaveSettings();
    if(b->isRunning()) {
        connect(this->b, SIGNAL(IShouldStop()), this, SLOT(ThreadShouldStop()));
        this->b->shouldIStop = true;
    } else {
        this->SaveSettings();
        this->b->terminate();
        delete this->b;
        delete ui;
    }
}

void Widget::hider(){
    mutex.lock();
    this->b->IShouldSleep = true;
    mutex.unlock();
    this->hide();

}

void Widget::setAutoStart(bool set){

    QDir autostartdir = QDir(QDir::homePath() % "/.config/autostart");
    if(!autostartdir.exists()){
        if(!autostartdir.mkdir(QDir::homePath() % "/.config/autostart")){
            qWarning() << "Autostart could not be activated: "+ QDir::homePath() % "/.config/autostart" +" could not be created!"  ;
        }
    }
    QFile *startfile = new QFile(QDir::homePath() % "/.config/autostart/fondos-bicentenario.desktop");
    if(set) {
        QByteArray FileData =  QString("[Desktop Entry]\nType=Application\nExec="%QApplication::applicationFilePath()%"\nHidden=false\nNoDisplay=false\nX-GNOME-Autostart-enabled=true\nName=Fondos-Bicentenario").toUtf8();
        if(!startfile->exists()) {
            if(startfile->open(QIODevice::WriteOnly)) {
                int returnStartFile = startfile->write(FileData);
                if(returnStartFile != -1 && returnStartFile == FileData.size()) {
                    startfile->flush();
                    startfile->close();
                } else {
                    qDebug() << "Failure while writing to autostart file";
                }
            }
        } else {
            if(startfile->size() != FileData.size()) {
                if(!startfile->remove()) {
                    qDebug() << "cant remove startup file!";
                }
                this->setAutoStart(set);
            }
        }
    } else {
        if(startfile->exists()) {
            if(!startfile->remove()) {
                qDebug() << "cant remove startup file!";
            }
        }
    }
}
void Widget::StartGui(){
    b->start();
    emit(this->shown(waitOnShow));
    this->waitOnShow.clear();
    mutex.lock();
    this->b->IShouldSleep = 0;
    mutex.unlock();
    this->show();
    if(!this->firstTime) {
        this->b->fileschanged = false;
        emit(toAddSignal(Files));
        this->CheckVersion();
        firstTime=true;
    }
}
void Widget::DeleteCurrentWP(){
    QString toRemove = this->currentWP();
    if(!QFileInfo(toRemove).isWritable()) {
        qWarning() << QString("Couldn't remove %1  -  insufficent permissions.").arg(toRemove);
    } else {
        if(toRemove != ""){
            QMessageBox *msgBox = new QMessageBox(this);
            msgBox->setText(trUtf8("Do you really want to delete this image from disk?"));
            msgBox->setStandardButtons(QMessageBox::Yes |QMessageBox::No);
            connect(msgBox, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(YesDeleteCurrentWP(QAbstractButton*)), Qt::AutoConnection);
            msgBox->show();
        }  else {
            return;
        }
    }
}
void Widget::YesDeleteCurrentWP(QAbstractButton* button){
    QString toRemove = this->currentWP();
    QString Pfad = QFileInfo(toRemove).absolutePath();

    QString Datei = QFileInfo(toRemove).fileName();
    if(button->text() ==  "&Yes") {

        this->RandomWallpaper();

        if(QFile(toRemove).remove()) {
            //   QStringList remlist;
            //    remlist.append(toRemove);
            //   toRem(toRemove);
            Tray->showMessage(trUtf8("Successful removed"), trUtf8("Filename: %1\nPath: %2").arg(Datei).arg(Pfad), QSystemTrayIcon::NoIcon, 5000);
        } else {
            if(QFile(toRemove).exists()){
                qWarning() << QString("Couldn't remove %1").arg(toRemove);
            } else {
                qWarning() << QString("Couldn't remove %1 because it is not existing anymore.").arg(toRemove);
            }
        }
    }
    if(button->text() ==  "&No") {
        Tray->showMessage(trUtf8("Not removed"), trUtf8("Filename: %1\nPath: %2").arg(Datei).arg(Pfad), QSystemTrayIcon::NoIcon, 2500);
    }
}


/**
 * Maneja el evento click sobre el icono de la 
 * @param Reason
 */
void Widget::TrayClick(QSystemTrayIcon::ActivationReason Reason){
  
    switch(Reason)
    {
    case QSystemTrayIcon::Trigger: //Click con botón izquierdo
        on_timeEdit_timeChanged(ui->timeEdit->time()/*tiempo de config*/); 
        this->RandomWallpaper(); //Selecciona fondo aleatorio
        break;
    case QSystemTrayIcon::Context:
        break;
    case QSystemTrayIcon::Unknown:
        break;
    case QSystemTrayIcon::DoubleClick:
        break;
    case QSystemTrayIcon::MiddleClick:
        break;

    }

}

void Widget::SaveWpStyle(int stylenum){
    QStringList styles;
    styles << "zoom" << "wallpaper" << "centered" << "scaled" << "stretched" << "spanned";
    QByteArray style = styles[stylenum].toUtf8();
    Settings->setValue("comboBox_wpstyle", stylenum);
    QProcess script(this);
    QString path = "gsettings set org.gnome.desktop.background picture-options \'"% style %"\'";
    //qDebug() << path;
    script.execute(path);
    if( script.error() == 0 ) {

        // qDebug() << script.errorString();
        GConfClient *gconf_client;
        gconf_client = gconf_client_get_default();

        if (gconf_client) {
            GError *error = 0;
            char  *path = g_strdup_printf("/desktop/gnome/background/picture_options");
            if (!gconf_client_set_string(gconf_client, path, style.data(), &error)) {
                qDebug() << style.data() << error->message;
                g_error_free(error);
            }
            g_free(path);
            g_object_unref(gconf_client);
        } else
            qDebug("Cannot get GConf2 client");
    }
    qDebug() << "SaveWpStyle";
}

void Widget::SaveSettings(){
    Settings->setValue("checkBox_changeWPOnStartup", ui->checkBox_changeWPOnStartup->checkState());
    Settings->setValue("checkBox_loadOnStartup", ui->checkBox_loadOnStartup->checkState());
    Settings->setValue("checkBox_loadOnStartup", ui->checkBox_loadOnStartup->checkState());
    Settings->setValue("timeEdit", ui->timeEdit->time());
    Settings->setValue("comboBox_wpstyle", ui->comboBox_wpstyle->currentIndex());
    if(Settings->isWritable() == false) {
        qDebug() << "Cant save Settings, because the path is not writable: " << Settings->fileName();
    }
    Settings->sync();
}


void Widget::LoadSettings(){   
    this->ui->comboBox_wpstyle->setCurrentIndex(Settings->value("comboBox_wpstyle").toInt());
    
    switch(Settings->value("checkBox_changeWPOnStartup").toInt())
    {
    case 2:
        ui->checkBox_changeWPOnStartup->setCheckState(Qt::Checked);
        this->setAutoStart(true);
        break;
    default: ui->checkBox_changeWPOnStartup->setCheckState(Qt::Unchecked);
        this->setAutoStart(false);
    }

    switch(Settings->value("checkBox_loadOnStartup").toInt())
    {
    case 2: 
        ui->checkBox_loadOnStartup->setCheckState(Qt::Checked);
        break;
    default: ui->checkBox_loadOnStartup->setCheckState(Qt::Unchecked);
    }
    
    ui->timeEdit->setTime(Settings->value("timeEdit").toTime());
    this->on_timeEdit_timeChanged(Settings->value("timeEdit").toTime());
    
    QStringList stringlist = Settings->value("listWidget_Dirs").toStringList();
    QStringList checkstatelist = Settings->value("listWidget_Dirs_flags").toStringList();
    

    if(!stringlist.isEmpty()) {

        for (int i = 0; i<stringlist.length(); i++) {
            QListWidgetItem *tmpwidget =  new QListWidgetItem(stringlist[i], ui->listWidget_Dirs, 0);
            tmpwidget->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            if(!checkstatelist.isEmpty() && !stringlist.isEmpty()) {

              if(checkstatelist.at(i) == "0")
                {
                    tmpwidget->setCheckState(Qt::Unchecked);
                } else {
                    tmpwidget->setCheckState(Qt::Checked);
                }
            } else {
                qDebug() << "index out of range: checkstatelist \nTry to fix the issue...";
                Settings->remove("listWidget_Dirs");
                Settings->sync();
                if(Settings->value("listWidget_Dirs").type() == QVariant::Invalid) {
                    qDebug() << "The issue should be fixed. \nRetry...";
                    this->DirDialog();
                } else {
                    qDebug() << "The issue couldn't' be fixed. \n Please contact the Developer!";
                }
            }
        }
    } else {        
        //this->DirDialog();
      
      //Cambiar WP al inicio
      ui->checkBox_changeWPOnStartup->setCheckState(Qt::Checked);
      this->setAutoStart(true);
      //Activar carga en el inicio de sesión
      ui->checkBox_loadOnStartup->setCheckState(Qt::Checked);
      //Asignar tiempo de cambio
      QTime * t = new QTime(12,0,0,0);
      ui->timeEdit->setTime(*t);
      this->on_timeEdit_timeChanged(*t);
      
      this->loadDirs();
      
    }
    this->readDates();
    this->firstload++;
}



void Widget::readDates() {
  QString file_path= "/usr/share/fondos-bicentenario/res/fechas";
  QFile file(file_path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QString errorMsg = trUtf8("¡No se encuentra el arhivo %1!").arg(file_path);
    QMessageBox::information(this, trUtf8("Error de lectura"),  errorMsg);
    return;
  }

  QTextStream stream(&file);
  QString line;
  while(!stream.atEnd()) {
    line = stream.readLine();
    this->datesList.append(line);
  }
}

void Widget::showEfeMsg(){
  int efeSize = this->datesList.count();
  srand((unsigned)time(0));
  int random_integer = (rand()%efeSize);
  float random_float = (rand()%100)/100.0;
  printf("float=%f\n",random_float);
  QStringList efem = this->datesList.at(random_integer).split("|");
  
  if(random_float < 0.1)
    Tray->showMessage(efem.at(1), efem.at(2), QSystemTrayIcon::NoIcon, 60000);
  
}

void Widget::loadDirs() { 

  
      
  QString dir_1810 = trUtf8("/usr/share/fondos-bicentenario/backgrounds/1810");

  QListWidgetItem *tmpwidget = new QListWidgetItem(dir_1810, ui->listWidget_Dirs, 0);
  tmpwidget->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
  tmpwidget->setCheckState(Qt::Unchecked);
  stringlist.append(dir_1810);
  checkstatelist.append("2");
  tmpwidget->setCheckState(Qt::Checked);
  Settings->setValue("listWidget_Dirs", stringlist);
  Settings->setValue("listWidget_Dirs_flags", checkstatelist);
  
  QString dir_1811 = trUtf8("/usr/share/fondos-bicentenario/backgrounds/1811");

  QListWidgetItem *tmpwidget2 = new QListWidgetItem(dir_1811, ui->listWidget_Dirs, 0);
  tmpwidget2->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
  tmpwidget2->setCheckState(Qt::Unchecked);
  stringlist.append(dir_1811);
  checkstatelist.append("2");
  tmpwidget2->setCheckState(Qt::Checked);
  Settings->setValue("listWidget_Dirs", stringlist);
  Settings->setValue("listWidget_Dirs_flags", checkstatelist);

  //this->DirChanged();

  this->b->IShouldSleep = false;
  this->b->fileschanged = false;
  
  this->SaveSettings();
}

void Widget::adjustSizer(){
    ui->btn_about->adjustSize();
    ui->btn_close->adjustSize();
    ui->btn_DirListAdd->adjustSize();
    ui->btn_DirListRem->adjustSize();
    ui->checkBox_changeWPOnStartup->adjustSize();
    ui->checkBox_loadOnStartup->adjustSize();
    ui->comboBox_wpstyle->adjustSize();
    ui->Display->adjustSize();
    ui->label->adjustSize();
    ui->label_changingInterval->adjustSize();
    ui->label_startoptions->adjustSize();
    ui->label_Style->adjustSize();
    ui->label_title1_listwidget_Dirs->adjustSize();
    ui->label_title2_listwidget_Dirs->adjustSize();
    ui->label_wpDir->adjustSize();
    ui->Preference->adjustSize();
    ui->ThumbCounter->adjustSize();
}

void Widget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        this->adjustSizer();
        break;
    default:
        break;
    }
}

void Widget::about()
{
    //QString License = "This program is free software: you can redistribute it and/or modify\nthe Free Software Foundation, either version 3 of the License, or\n(at your option) any later version.\n\nThis package is distributed in the hope that it will be useful,\nbut WITHOUT ANY WARRANTY; without even the implied warranty of\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\nGNU General Public License for more details.\n\nYou should have received a copy of the GNU General Public License\nalong with this program.  If not, see <http://www.gnu.org/licenses/>.";
    QMessageBox *about = new QMessageBox(this);
    QString name = QApplication::applicationName();
    QString version = QApplication::applicationVersion();
    QString versiontr = trUtf8("version");
    QString developedstr = trUtf8("Developed by");
    QString poweredby = trUtf8("Powered by");
    

    QImage logo_image;
    logo_image.load(":/images/logo.jpg");
    QLabel *logo_label = new QLabel(about);
    logo_label->setPixmap(QPixmap::fromImage(logo_image));
    logo_label->setGeometry (35, about->height() + 10, logo_image.width(), logo_image.height());

    QLabel *title = new QLabel(about);    
    title->setText(name);
    QFont font = about->font();
    font.setBold(true);
    title->setFont(font);
    title->setStyleSheet("qproperty-alignment: AlignCenter;");
    title->setGeometry(logo_label->x(), logo_label->y()+120, 300, title->height());

    QLabel *versionLabel = new QLabel(about);    
    versionLabel->setText(versiontr % ":" % version);
    versionLabel->setGeometry(title->x(), title->y() + 20, 300, versionLabel->height());
    versionLabel->setStyleSheet("qproperty-alignment: AlignCenter;");
    
    
    QLabel *website_launchpad = new QLabel(about);
    website_launchpad->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    website_launchpad->setOpenExternalLinks(true);
    website_launchpad->setText("<a href=\"http://bicentenario.cenditel.gob.ve\">Proyecto Bicentenario</a>");
    website_launchpad->setGeometry(title->x(), title->y() + 60, 300, website_launchpad->height());
    website_launchpad->setStyleSheet("qproperty-alignment: AlignCenter;");

    QLabel *developed = new QLabel(about);    
    developed->setText(developedstr % ":");
    developed->setGeometry(website_launchpad->x(), website_launchpad->y() + 30, 300, versionLabel->height());
    developed->setStyleSheet("qproperty-alignment: AlignCenter;");

    QLabel *email = new QLabel(about);
    email->setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::TextSelectableByMouse);
    email->setOpenExternalLinks(true);
    email->setText("<a href=\"mailto:jruiz@cenditel.gob.ve\">José Ruiz (jruiz@cenditel.gob.ve)</a>");
    email->setGeometry(developed->x(), developed->y() + 25, 300, email->height());
    email->setStyleSheet("qproperty-alignment: AlignCenter;");

    QLabel *email2 = new QLabel(about);
    email2->setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::TextSelectableByMouse);
    email2->setOpenExternalLinks(true);
    email2->setText("<a href=\"mailto:jquintero@cenditel.gob.ve\">Joger Quintero(jquintero@cenditel.gob.ve)</a>");
    email2->setGeometry(email->x(), email->y() + 25, 300, email->height());
    email2->setStyleSheet("qproperty-alignment: AlignCenter;");

    QLabel *powered = new QLabel(about);    
    powered->setText(poweredby % ": Cortina");
    powered->setGeometry(email2->x(), email2->y() + 60, 300, versionLabel->height());
    powered->setStyleSheet("qproperty-alignment: AlignCenter;");

    about->setText("                      Fundación CENDITEL\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    about->setWindowTitle(trUtf8("about"));

        
    /*Tamaño del MessageBox*/
    QSpacerItem* horizontalSpacer = new QSpacerItem(340, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    QGridLayout* layout = (QGridLayout*)about->layout();
    layout->addItem(horizontalSpacer, layout->rowCount(), 0, 1, layout->columnCount());
    
    about->show();
}


QString Widget::currentWP(){
    QString wp2;
    gchar *wp;
    QProcess script(this);
    QStringList arguments;
    arguments << "get" << "org.gnome.desktop.background" << "picture-uri";
    //connect(&script, SIGNAL(error(QProcess::ProcessError)), &script, SLOT(kill()));
    QProcess script2(this);

    if (script2.execute("which gsettings > /dev/null") == 0){
        script.start("gsettings", arguments, QIODevice::ReadWrite);
    }else{
        GConfClient *gconf_client = gconf_client_get_default();

        wp = gconf_client_get_string(gconf_client, "/desktop/gnome/background/picture_filename", NULL);
        wp2 = wp;
        g_object_unref(gconf_client);
        return wp2;
    }
    qDebug() << "currentWP";
    // Continue reading the data until EOF reached
    QByteArray data;

    while(script.waitForReadyRead())
        data.append(script.readAll());

    qDebug() << QUrl(data.data()).toString().remove(0,8).trimmed().remove('\'');
    wp2 = QUrl(data.data()).toString().remove(0,8).trimmed().remove('\'');

    return wp2;
}

/**
 * Cambia el fondo de pantalla por el seleccionado
 * @param tempwpfile
 */
void Widget::setWP(QString tempwpfile){
  
    // Alterna iconos 
    if(WidgetIcon == false) {
        this->setWindowIcon(QIcon(QString::fromUtf8(":/icons/fondos-bicentenario.png")));
        this->Tray->setIcon(QIcon(QString::fromUtf8(":/icons/fondos-bicentenario.png")));
        WidgetIcon = true;
    } else {
        WidgetIcon = false;
        this->setWindowIcon(QIcon(QString::fromUtf8(":/icons/fondos-bicentenario2.png")));
        this->Tray->setIcon(QIcon(QString::fromUtf8(":/icons/fondos-bicentenario2.png")));
    }
    
    QProcess script(this);
    QString path = "gsettings set org.gnome.desktop.background picture-uri \'"% QUrl::fromLocalFile(tempwpfile).toEncoded(QUrl::None) %"\'";
    qDebug() << path;
    QStringList arguments;
    arguments << path;

    QProcess script2(this);

    if (script2.execute("which gsettings") == 0){
        qDebug() << "setWP the new way";
        script.execute(path);
    }else{
        qDebug() << "setWP the old way";
        QByteArray wpfile = tempwpfile.toUtf8();
        GConfClient *gconf_client = gconf_client_get_default ();
        if (gconf_client) {
            GError *error = 0;
            char  *path = g_strdup_printf("/desktop/gnome/background/picture_filename");
            if (!gconf_client_set_string(gconf_client, path, wpfile.data(), &error)) {
                qDebug() << wpfile.data() << error->message;
                g_error_free(error);
            }
            g_free(path);
            g_object_unref(gconf_client);
        } else
            qDebug("Cannot get GConf2 client");
    }
    
    // Comentado por ahora no permite eliminar imágnes
    
//    if(QFileInfo(this->currentWP()).isWritable()) {
//        removeAction->setEnabled(true);
//    } else {
//        removeAction->setEnabled(false);
//    }
    
    setTooltipText(); //Manejador de tooltips del icono de la bandeja
}

void Widget::addToList(const QImage& image,const QString& filename){
    if(this->b->fileschanged == false) {
        QString fileDir = QFileInfo(filename).absolutePath();
        QString filename2 = QFileInfo(filename).fileName();
        QString thumb = fileDir % "/.thumb_" % filename2;
        QPixmap Pixmap;
        mutex.tryLock();
        if(image.isNull()) {            
            Pixmap = QPixmap(thumb);
        } else {
            try{
                Pixmap = QPixmap::fromImage(image);
            } catch (...) {
                qDebug() << filename << " couldn't be loaded!";
                return;
            }
        }
        ItemList.append(filename);
        mutex.unlock();
        QPixmap & refPixmap = Pixmap;
        QIcon Icon = QIcon(refPixmap);
        const QIcon & refIcon = Icon;
        QString filenameWithoutPath = QFileInfo(filename).fileName();
        this->ui->listWidget->addItem(new QListWidgetItem(refIcon, filenameWithoutPath, this->ui->listWidget,0));
        Pixmap = QPixmap();
        Icon = QIcon();
        Counter = this->ItemList.count();
        this->ui->ThumbCounter->setNum(Counter);
    }
}
void Widget::YesDeleteSelectedWP(QAbstractButton* button){	
    if(button->text() ==  "&Yes") {
        QString toRemove = this->ItemList.at(this->ui->listWidget->currentIndex().row());
        if(!QFileInfo(toRemove).isWritable()) {
            qWarning() << QString("Couldn't remove %1  -  insufficent permissions.").arg(toRemove);
        } else {
            QString Pfad = QFileInfo(toRemove).absolutePath();
            QString Datei = QFileInfo(toRemove).fileName();
            if(QFile(toRemove).remove()) {
                //Tray->setToolTip(trUtf8("Filename: %1\nPath: %2").arg(Datei).arg(Pfad));
                Tray->showMessage(trUtf8("Successful removed"), trUtf8("Filename: %1\nPath: %2").arg(Datei).arg(Pfad), QSystemTrayIcon::NoIcon, 5000);
            } else {
                if(QFile(toRemove).exists()){
                    qWarning() << QString("Couldn't remove %1").arg(toRemove);
                } else {
                    qWarning() << QString("Couldn't remove %1 because it is not existing anymore.").arg(toRemove);
                }
            }
        }
    }
}

void Widget::DeleteSelectedWP(){
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setText(trUtf8("Do you really want to delete this image from disk?"));
    msgBox->setStandardButtons(QMessageBox::Yes |QMessageBox::No);
    connect(msgBox, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(YesDeleteSelectedWP(QAbstractButton*)));
    msgBox->show();
}
void Widget::listWidget_customContextMenuRequested(const QPoint & pointer){
    QAction *Aktion = new QAction(trUtf8("delete"), ui->listWidget);
    connect(Aktion, SIGNAL(triggered()), this, SLOT(DeleteSelectedWP()));
    QMenu menu(ui->listWidget);
    menu.addAction( Aktion );
    menu.exec(ui->listWidget->mapToGlobal( pointer ));
}
void Widget::listWidget_doubleClicked(QModelIndex i)
{
    on_timeEdit_timeChanged(ui->timeEdit->time());
    QString Zeile = ItemList.at(i.row());
    if(QFile(Zeile).exists()) {
        this->setWP(Zeile);
        //  qDebug() << "Changed Wallpaper to:" << Zeile;
    } else {
        qDebug() << "File not found! " << Zeile;
    }
}

void Widget::DirDialog(){
    FD = new QFileDialog(NULL, Qt::Dialog);
    FD->setAcceptMode(QFileDialog::AcceptOpen);
    FD->setLabelText(QFileDialog::Accept, trUtf8("add"));
    FD->setLabelText(QFileDialog::FileType, trUtf8("File typ"));
    FD->setLabelText(QFileDialog::FileName, trUtf8("Directory name"));
    FD->setLabelText(QFileDialog::Reject, trUtf8("close"));
    FD->setWindowTitle(trUtf8("Choose directory..."));
    connect(this->FD, SIGNAL(finished(int)), this, SLOT(FDChangeDir(int)));
    FD->setDirectory(QDir::home());
    FD->setFileMode(QFileDialog::DirectoryOnly);
    FD->exec();

}

void Widget::FDChangeDir(int i){
    //   int add = 0;
    if(i != 0) {
        QStringList stringlist = Settings->value("listWidget_Dirs").toStringList();
        QStringList checkstatelist = Settings->value("listWidget_Dirs_flags").toStringList();
        foreach(QString selectedFile, this->FD->selectedFiles())
        {

            for(int i = 0; i<stringlist.length(); i++) {
                if(selectedFile.startsWith(stringlist[i]) == true  &&  checkstatelist[i] == "2") {
                    //  add++;
                    QString dublicateString = trUtf8("%1 is already monitored through %2!")
                                              .arg(selectedFile).arg(stringlist[i]);
                    QMessageBox::information(this, trUtf8("dublicate detected"),  dublicateString);
                    return;
                } else if(stringlist[i] == selectedFile){
                    //  add++;
                    QString dublicateString = trUtf8("%1 is already monitored!").arg(selectedFile);
                    QMessageBox::information(this, trUtf8("dublicate detected"),  dublicateString);
                    return;
                }
            }
            // if(add == 0) {
            QListWidgetItem *tmpwidget = new QListWidgetItem(selectedFile, ui->listWidget_Dirs, 0);
            tmpwidget->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            tmpwidget->setCheckState(Qt::Unchecked);
            stringlist.append(selectedFile);
            checkstatelist.append("0");
            Settings->setValue("listWidget_Dirs", stringlist);
            Settings->setValue("listWidget_Dirs_flags", checkstatelist);
            //this->DateiListe();
            this->DirChanged();
            /*if(Files.count() != 0) {
                for(int i = 0; i < this->ItemList.count(); i++)    {
                    if(Files.contains(ItemList[i])) {
                        Files.removeAll(ItemList[i]);
                    }
                }
            }*/
            this->b->IShouldSleep = false;
            this->b->fileschanged = false;
            //this->DateiListe();
            //emit(toAddSignal(Files));
            this->SaveSettings();
            this->FD->deleteLater();

            // }
        }
    }
}


void Widget::DirListRem(){

    this->b->IShouldSleep = true;
    DateiListe();
    //if(ui->listWidget_Dirs->selectedItems().count() != 0) {
    QString removedItem =  ui->listWidget_Dirs->selectedItems()[0]->text();

    QStringList stringlist = Settings->value("listWidget_Dirs").toStringList();
    QStringList checkstatelist = Settings->value("listWidget_Dirs_flags").toStringList();
    if(this->ui->listWidget_Dirs->count() != 0) {
        for(int i = 0; i < stringlist.count(); i++) {
            if(stringlist[i] == removedItem) {

                helper *thumb_del_thread = new helper();
                thumb_del_thread->autoDelete();
                thumb_del_thread->path = stringlist[i];
                thumb_del_thread->recursiv = checkstatelist[i];
                this->watcher->removePath(removedItem);
                QThreadPool::globalInstance()->start(thumb_del_thread);

                delete ui->listWidget_Dirs->selectedItems()[0];
                stringlist.removeAt(i);
                checkstatelist.removeAt(i);

            }
        }
        if(this->ui->listWidget_Dirs->count() != 0) {
            this->b->fileschanged = true;
        }else{
            this->watcher->removePath(removedItem);
            this->ItemList.clear();
            this->ui->listWidget->clear();
            this->Counter = 0;
            this->ui->ThumbCounter->setNum(Counter);
            this->Files.clear();
        }
        Settings->setValue("listWidget_Dirs", stringlist);
        Settings->setValue("listWidget_Dirs_flags", checkstatelist);
        QStringList tmpremFiles;
        QDir verzeichnis = QDir(removedItem);
        const QStringList & SubDirs = verzeichnis.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::NoSort);
        foreach(QString File, Files) {
            if(SubDirs.count() != 0){
                int helperCount = 0;
                for(int i = 0; i<SubDirs.count(); i++) {
                    if(!File.contains(verzeichnis.path() + "/" + SubDirs[i]))
                    {
                        helperCount++;
                    }
                }
                int bla = SubDirs.count() - helperCount;
                if(bla == 0 && File.contains(removedItem)) {
                    tmpremFiles.append(File);
                }

            }else{
                if(File.contains(removedItem)) {
                    tmpremFiles.append(File);
                }
            }
        }
        if(tmpremFiles.count() != 0){
            DateiListe();
            const QStringList & remFiles = tmpremFiles;
            toRem(remFiles);
        }
        /*} else {
   Settings->remove("listWidget_Dirs");
   Settings->remove("listWidget_Dirs_flags");
   this->ItemList.clear();
   this->ui->listWidget->clear();
   this->Counter = 0;
   this->ui->ThumbCounter->setNum(Counter);
   this->Files.clear();
                }*/

    }

}
void Widget::CheckVersion(){
    QUrl url = QUrl("http://eric32.er.funpic.de/fondos-bicentenario/natty_current");
    QNetworkAccessManager *manager = new QNetworkAccessManager();
    connect(manager, SIGNAL(finished(QNetworkReply*))
            , SLOT(downloadFinished(QNetworkReply*)));

    QString var(getenv("http_proxy"));
    QRegExp regex("(http://)?(.*):(\\d*)/?");
    int pos = regex.indexIn(var);

    if(pos > -1){
        QString host = regex.cap(2);
        int port = regex.cap(3).toInt();
        QNetworkProxy proxy(QNetworkProxy::HttpProxy, host, port);
        QNetworkProxy::setApplicationProxy(proxy);
    }

    //qDebug() << QNetworkProxy::applicationProxy().hostName();
    QNetworkRequest request = QNetworkRequest(url);
    manager->get(request);
}

void Widget::downloadFinished(QNetworkReply* reply){   
    QUrl url = reply->url();
    if (reply->error()) {
        qDebug() << "Version check failed: " % reply->errorString();
        /*fprintf(stderr, "Version check failed \n",
                url.toEncoded().constData(),
                qPrintable(reply->errorString()));*/
    } else if(reply->isReadable() && reply->size() != 0) {
        QByteArray buf = reply->readAll();
        QString inhalt;
        for(int i = 0; i<buf.length(); i++) {
            inhalt += buf.at(i);
        }
        inhalt = inhalt.trimmed();
        QString InstalledVersion = QApplication::applicationVersion();
        if(inhalt != InstalledVersion) {
            QStringList splittedlocal = InstalledVersion.split(".");
            QStringList splittedRemote = inhalt.split(".");
            int local = 0;
            int remote = 0;
            int i = 0;
            foreach(QString num, splittedlocal) {
                if(i == 0)
                {
                    local = local + num.toInt() * 100;
                }
                if(i == 1)
                {
                    local = local + num.toInt() * 10;
                } else{
                    local = local + num.toInt();
                }
                i++;
            }
            i = 0;
            foreach(QString num, splittedRemote) {
                if(i == 0)
                {
                    remote = remote + num.toInt() * 100;
                }
                if(i == 1)
                {
                    remote = remote + num.toInt() * 10;
                } else{
                    remote = remote + num.toInt();
                }
                i++;
            }
            //qDebug() << "remote=" << remote << "\n" << "local=" << local;
            if(remote > local) {
                QString versionmsg = trUtf8("%1 is avaible!\nClick on the about button to see where you can get it!").arg(inhalt);
                QMessageBox::information(this, trUtf8("Update avaible!"), versionmsg);
            }
        }
    }
    return;
    reply->deleteLater();
}

void Widget::CreateUI(){
    this->adjustSizer();
    connect(ui->btn_DirListAdd, SIGNAL(clicked()), this, SLOT(DirDialog()));
    connect(ui->btn_DirListRem, SIGNAL(clicked()), this, SLOT(DirListRem()));
    intervalTimer = new QTimer(this);
    connect(intervalTimer, SIGNAL(timeout()), this, SLOT(RandomWallpaper()));
    ui->Tab->setCurrentIndex(0);
}
QStringList Widget::DateiListeNeu(){
    QStringList FilesNeu;
    QStringList stringlist = Settings->value("listWidget_Dirs").toStringList();
    QStringList checkstatelist = Settings->value("listWidget_Dirs_flags").toStringList();
    if(stringlist.length() != 0) {
        if(Settings->value("listWidget_Dirs").toStringList().length() != 0) {
            for (int i = 0; i<stringlist.length(); i++) {
                QDir tempDir = QDir(stringlist.at(i));
                QDir & verzeichnis = tempDir;
                QStringList filters;
                const QStringList & SubDirs = verzeichnis.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::NoSort);
                filters << "*.jpg" << "*.jpeg" << "*.gif" << "*.png" << "*.svg";
                verzeichnis.setNameFilters(filters);
                int SubDirscount = SubDirs.count();

                if(checkstatelist.at(i) == "0") {
                    SubDirscount = 0;
                }
                if(SubDirscount >= 1) {
                    foreach(QString dir, SubDirs) {
                        verzeichnis.setPath(stringlist.at(i) % "/" % dir);
                        foreach(QString Datei,verzeichnis.entryList(verzeichnis.nameFilters(), QDir::Files | QDir::Readable, QDir::NoSort)) {
                            FilesNeu.append(verzeichnis.path() % "/" % Datei);
                        }}

                    verzeichnis.setPath(stringlist.at(i));
                    foreach(QString Datei,verzeichnis.entryList(verzeichnis.nameFilters(), QDir::Files | QDir::Readable, QDir::NoSort)) {
                        FilesNeu.append(verzeichnis.path() % "/" % Datei);
                    }

                } else {
                    foreach(QString Datei,verzeichnis.entryList(verzeichnis.nameFilters(), QDir::Files | QDir::Readable, QDir::NoSort)) {
                        FilesNeu.append(verzeichnis.path() % "/" % Datei);
                    }
                }
            }
        }
    }

    return FilesNeu;
}

void Widget::DateiListe(){
    Files.clear();
    if(!Settings->value("listWidget_Dirs").toStringList().isEmpty()) {
        QStringList stringlist = Settings->value("listWidget_Dirs").toStringList();
        QStringList checkstatelist = Settings->value("listWidget_Dirs_flags").toStringList();
        QStringList Hidden;
        QStringList Hidden2;
        if(stringlist.length() != 0) {
            for (int i = 0; i<stringlist.length(); i++) {
                QDir tempDir = QDir(stringlist.at(i));
                QDir & verzeichnis = tempDir;
                QStringList filters;
                const QStringList & SubDirs = verzeichnis.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::NoSort);
                filters << "*.jpg" << "*.jpeg" << "*.gif" << "*.png" << "*.svg";
                verzeichnis.setNameFilters(filters);
                int SubDirscount = SubDirs.count();
                if(!checkstatelist.isEmpty()) {

                    if(checkstatelist.at(i) == "0") {
                        SubDirscount = 0;
                    }
                    if(SubDirscount >= 1) {
                        if(!watcher->directories().contains(verzeichnis.path())) {
                            watcher->addPath(verzeichnis.path());
                        }

                        foreach(QString Datei,verzeichnis.entryList(verzeichnis.nameFilters(), QDir::Files | QDir::Readable, QDir::NoSort)) {
                            Files.append(verzeichnis.path() % "/" % Datei);
                        }
                        foreach(QString dir, SubDirs) {
                            verzeichnis.setPath(stringlist.at(i) % "/" % dir);
                            if(!watcher->directories().contains(verzeichnis.path())) {
                                watcher->addPath(verzeichnis.path());
                            }
                            foreach(QString Datei,verzeichnis.entryList(verzeichnis.nameFilters(), QDir::Files | QDir::Readable, QDir::NoSort)) {
                                Files.append(verzeichnis.path() % "/" % Datei);
                            }

                        }

                    } else {
                        if(!watcher->directories().contains(verzeichnis.path())) {
                            watcher->addPath(verzeichnis.path());
                        }

                        foreach(QString Datei,verzeichnis.entryList(verzeichnis.nameFilters(), QDir::Files | QDir::Readable, QDir::NoSort)) {
                            Files.append(verzeichnis.path() % "/" % Datei);
                        }
                    }
                }    else {
                    qFatal("index out of range: checkstatelist");
                }
            }
        }
    }
    if(Files.length() == 0) {
        qDebug() << "Can't work without wallpapers!";
        return;
    }
}
void Widget::ImageSaved(bool saved){
    this->ImageSavedWatcher = saved;
}
/*
void Widget::LoopWatcher(){
    qDebug() << Started;
qDebug() << QTime::currentTime().addSecs(-5);
    while( Started < QTime::currentTime().addSecs(-5) ) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
    watcher->
    Started = QTime::currentTime();
}
*/
void Widget::DirChanged(){
    // this->LoopWatcher();
    if(this->ImageSavedWatcher == false) {
        // this->b->fileschanged = true;
        QTime dieTime = QTime::currentTime().addMSecs(100);
        /*qDebug() << "Loop start..";
        qDebug() << QTime::currentTime().msec();
          qDebug() <<  dieTime.msec();
          */
        while( QTime::currentTime() < dieTime ) {
        }
        /*qDebug() << "Loop end..";
        qDebug() << QTime::currentTime().msec();
          qDebug() <<  dieTime.msec();*/
        const QStringList & NeueListe = DateiListeNeu();
        const QStringList & AlteListe = Files;
        const int & NeueAnzahl = NeueListe.count();
        const int & AlteAnzahl = AlteListe.count();
        QStringList addy;
        const QStringList & refaddy = addy;
        QStringList remy;
        const QStringList & refremy = remy;
        addy.clear();
        remy.clear();

        qDebug() << "NeueAnzahl: " << NeueAnzahl;
        qDebug() << "AlteAnzahl: " << AlteAnzahl;
        qDebug() << "----";
        int NeueAnzahltmp = NeueAnzahl;
        if(NeueAnzahl > AlteAnzahl) {
            // Datei wurde hinzugefügt:
            foreach(QString Datei, NeueListe)
            {
                if(NeueAnzahltmp != AlteAnzahl) {
                    if(AlteListe.indexOf(Datei) == -1) {
                        addy.append(Datei);
                        NeueAnzahltmp--;
                    }
                }
            }
            for(int i = 0; i < this->ItemList.count(); i++)    {
                if(refaddy.contains(ItemList[i])) {
                    addy.removeAll(ItemList[i]);
                }
            }
            if(!this->isHidden()) {
                this->b->fileschanged = false;
                emit(toAddSignal(refaddy));
                this->Files.append(refaddy);
            } else {
                waitOnShow.append(refaddy);
                this->Files.append(refaddy);
            }
        } else if(NeueAnzahl < AlteAnzahl) {
            // Datei wurde gelöscht:
            foreach(QString Datei, AlteListe)
            {
                if(NeueListe.indexOf(Datei) == -1) {
                    remy.append(Datei);
                }
            }
            this->toRem(refremy);
        } else if(NeueAnzahl == AlteAnzahl) {
            foreach(QString Datei, AlteListe)
            {
                if(!QFile(Datei).exists()) {
                    remy.append(Datei);
                }/*else{
                    qDebug()<< this->ItemList.contains(QFileInfo(Datei).absoluteFilePath());
                     qDebug()<< refaddy.contains(Datei);
                    if(NeueListe.contains(Datei) && !refaddy.contains(Datei) && !this->ItemList.contains(QFileInfo(Datei).absoluteFilePath())){
                        addy.append(Datei);
                        if(!this->isHidden()) {
                            this->b->fileschanged = false;
                            emit(toAddSignal(refaddy));
                            this->Files.append(refaddy);
                        } else {
                            waitOnShow.append(refaddy);
                            this->Files.append(refaddy);
                        }
                    }
                }*/
            }
            this->toRem(refremy);
        }
    }
    //qDebug() << "DirChanged!";
    // qDebug() << watcher->directories();
}

void Widget::toRem(const QStringList & Liste){
    // emit(ThreadShouldSleep(5));
    foreach(QString remDatei, Liste) {
        for(int i = 0; i < ItemList.length(); i++) {
            if(this->ItemList.at(i) == remDatei) {
                QString fileDir = QFileInfo(remDatei).absolutePath();
                QString filename2 = QFileInfo(remDatei).fileName();
                QString thumb = fileDir % "/.thumb_" % filename2;
                delete this->ui->listWidget->item(i);
                this->ItemList.removeAt(i);
                this->ImageSavedWatcher = true;
                QFile(thumb).remove();
                Counter--;
                this->ui->ThumbCounter->setNum(Counter);
            }
        }
    }
    this->DateiListe();
    this->b->IShouldSleep = false;
    this->ImageSavedWatcher = false;
}


/**
 * Selecciona un Fondo de pantalla de forma alaeatoria
 */
void Widget::RandomWallpaper(){
    QString WP;
    const int & reflenght =   this->Files.length(); //Obtiene cantidad de imágenes
    
    //Si hay imágenes selecciona una aleatoriamente
    if(reflenght != 0) {
        //if(Settings->value("randomOrNot") == 2) {
        srand((unsigned)time(0));
        int random_integer = (rand()%reflenght);
        const int & refrandom_integer = random_integer;
        WP = Files[refrandom_integer];
        /* } else {
            if((current) != reflenght) {
                WP = Files[current];
                current++;
            } else {
                current = 0;
                WP = Files[current];
            }
       }*/
    } else { // Si no hay imágenes
        QStringList stringlist = Settings->value("listWidget_Dirs").toStringList();
        if(!stringlist.isEmpty()) {
            //this->DateiListe();
            if(Files.length() == 0) {
                return;
            }
            this->RandomWallpaper();
        }
    }
    if(QFile(WP).exists()) {
        this->setWP(WP);
    } else {
        qDebug() << "File not found! " << WP;
    }
}

/**
 * Controla la actualización del tiempo de cambio del fondo
 * @param date
 */

void Widget::on_timeEdit_timeChanged(QTime date)
{
    float hours2ms = 0;
    float min2ms = 0;
    float sec2ms = 0;
    if(date.hour() > 0){
        hours2ms = ((date.hour() * 60) * 60) * 1000;
    }
    if(date.minute() > 0){
        min2ms = (date.minute() * 60) * 1000;
    }
    if(date.second() > 0){
        sec2ms = date.second() * 1000;
    }
    int msecs = hours2ms + min2ms + sec2ms; //date(tiempo de config) en milisegundos
    //qDebug() << msecs;

    if(intervalTimer->isActive())
    {
        if(msecs != 0){
            intervalTimer->setInterval(msecs);
        } else {
            intervalTimer->stop();
        }
    } else {
        if(msecs != 0){
            intervalTimer->start(msecs);
        }
    }
}

