
%Module blur.Stonegui 0

%Import QtCore/QtCoremod.sip
%Import QtGui/QtGuimod.sip
%Import QtNetwork/QtNetworkmod.sip

%Import blur/blurqt.sip

%DefaultSupertype sip.wrapper

%Include busywidget.sip
%Include configdbdialog.sip
%Include extgraphicsscene.sip
%Include exttreewidgetitem.sip
%Include fieldcheckbox.sip
%Include fieldlineedit.sip
%Include fieldspinbox.sip
%Include fieldtextedit.sip
%Include graphitedialog.sip
%Include graphitesource.sip
%Include graphitesourceswidget.sip
%Include graphiteoptionswidget.sip
%Include graphitewidget.sip
%Include htmlhighlighter.sip
%Include lostconnectiondialog.sip
%Include modelgrouper.sip
%Include modeliter.sip
%Include qvariantcmp.sip
%Include recordcombo.sip
%Include recorddelegate.sip
%Include recorddrag.sip
%Include recordlistmodel.sip
%Include recordlistview.sip
%Include recordtreeview.sip
%Include recordsupermodel.sip
%Include richtexteditor.sip
%Include supermodel.sip

%ModuleHeaderCode
#include "iconserver.h"
%End

class ConnectionWatcher : QObject
{
public:
	ConnectionWatcher();

	LostConnectionDialog * dialog();
	
	static ConnectionWatcher * connectionWatcher();
public slots:
	void connectionLost();
	void connected();
};

void initStoneGui();

QPixmap icon( const QString & name );
