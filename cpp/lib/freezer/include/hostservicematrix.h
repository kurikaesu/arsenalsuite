
#ifndef HOST_SERVICE_MATRIX_H
#define HOST_SERVICE_MATRIX_H

#include <qaction.h>
#include <qtimer.h>
#include <qmainwindow.h>

#include "host.h"
#include "hostservice.h"
#include "hoststatus.h"
#include "service.h"

#include "recordsupermodel.h"
#include "recordtreeview.h"

#include "afcommon.h"

#include "ui_hostservicematrixwidgetui.h"

class HostServiceModel;

class FREEZER_EXPORT HostServiceModel : public RecordSuperModel
{
Q_OBJECT
public:
	HostServiceModel( QObject * parent = 0 );

	HostService findHostService( const Host & host, int column ) const;
	HostService findHostService( const QModelIndex & );
	QVariant serviceData ( const Host &, int column, int role ) const;
	Service serviceByColumn( int column ) const;

	void setHostFilter( const QString &, bool cascading=false );
	void setServiceFilter( const QString &, bool cascading=false );
	void setStatusFilter( const QString &, bool cascading=false );
	void setOSFilter( const QString &, bool cascading=false );
	void setUserLoggedFilter( bool, bool, bool cascading=false );
	
	void setHostList( HostList hosts );
	void updateHosts( HostList hosts );
	void refreshIndexes( QModelIndexList indexes );
	
public slots:
	void updateServices();

	void hostServicesAdded(RecordList);
	void hostServicesRemoved(RecordList);
	void hostServiceUpdated(Record up, Record);

protected:
	bool hostFiltering;
	QString currentHostFilter;

	bool statusFiltering;
	QString currentStatusFilter;

	bool osFiltering;
	QString currentOsFilter;

	bool userFiltering;

	HostList currentFilteredList;

	HostStatusList mStatuses;
	HostServiceList mHostServices;
	QMap<Host,HostServiceList> mHostServicesByHost;
	ServiceList mServices;
};

class FREEZER_EXPORT HostServiceMatrix : public RecordTreeView
{
Q_OBJECT
public:
	HostServiceMatrix( QWidget * parent = 0 );
	HostServiceModel * getModel() const;

	bool hostFilterCS() const { return mHostFilterCS; }
	bool serviceFilterCS() const { return mServiceFilterCS; }
	
public slots:
	void setHostFilter( const QString & );
	void setServiceFilter( const QString & );
	void setStatusFilter( const QString & );
	void setOSFilter( const QString & );
	void setUserLoggedFilter( int );

	void setUserLoggedType( bool );

	void hostTimerExpired();
	void statusTimerExpired();
	void osTimerExpired();
	void setHostFilterCS( bool cs );
	void setServiceFilterCS( bool cs );
	
	void slotShowMenu( const QPoint & pos, const QModelIndex & underMouse );

	void updateServices();
protected:
	QTimer* hostFTimer;
	QTimer* statusFTimer;
	QTimer* osFTimer;

	bool useLoggedIn;
	bool userFiltering;

	QString mHostFilter, mServiceFilter, mHostStatusFilter, mHostOsFilter;
	bool mHostFilterCS, mServiceFilterCS;
	HostServiceModel * mModel;
};

class FREEZER_EXPORT HostServiceMatrixWidget : public QWidget, public Ui::HostServiceMatrixWidgetUi
{
Q_OBJECT
public:
	HostServiceMatrixWidget( QWidget * parent = 0 );

	void refreshHostGroups();

public slots:
	void setShowMyGroupsOnly( bool );
	void showOptionsMenu();
	void performHostGroupRefresh();
	void hostGroupChanged( const Record & hgr );
	void newService();

	void saveHostGroup();
	void manageHostLists();

protected:
	HostServiceMatrix * mView;
	bool mHostGroupRefreshPending;
	HostList hostList() const;

	QAction * mShowMyGroupsOnlyAction, * mManageGroupsAction, * mSaveGroupAction;
	virtual bool eventFilter( QObject * o, QEvent * e );
	
};

class FREEZER_EXPORT HostServiceMatrixWindow : public QMainWindow
{
Q_OBJECT
public:
	HostServiceMatrixWindow( QWidget * parent = 0 );
	
};

#endif // HOST_SERVICE_MATRIX_H
