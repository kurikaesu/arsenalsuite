
%Module blur.Freezer

%Import QtCore/QtCoremod.sip
%Import QtGui/QtGuimod.sip
%Import blur/classes.sip
%Import blur/stonegui.sip

%DefaultSupertype sip.wrapper

%ModuleHeaderCode
#include "afcommon.h"
%End

void exploreFile( QString path );

%Include assfreezermenus.sip
%Include assfreezerview.sip
%Include graphiteview.sip
%Include joblistwidget.sip
%Include jobsettingswidget.sip
%Include jobsettingswidgetplugin.sip
%Include jobviewerplugin.sip
%Include hostviewerplugin.sip
%Include frameviewerplugin.sip
%Include multiframeviewerplugin.sip
%Include joberrorswidgetplugin.sip
%Include jobframestabwidgetplugin.sip
%Include framenthdialog.sip
%Include mainwindow.sip

%PostInitialisationCode
registerBuiltinCustomJobSettingsWidgets();
%End
