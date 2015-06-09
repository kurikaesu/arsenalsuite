
#ifndef HOST_LIST_WIDGET_H
#define HOST_LIST_WIDGET_H

#include <qstring.h>

#include "service.h"

#include "assfreezerview.h"

#include "ui_hostlistwidgetui.h"

class QAction;
class QMenu;

class FilterEdit;
class RecordTreeView;

class FreezerWidget;

void clearHostErrorsAndSetOffline( HostList hosts, bool offline);
QString verifyKeyList( const QString & list, Table * table );
QList<uint> verifyKeyList( const QList<uint> & list, Table * table );

class FREEZER_EXPORT HostListWidget : public FreezerView, public Ui::HostListWidgetUI
{
Q_OBJECT
public:
	HostListWidget(QWidget * parent);
	~HostListWidget();

	virtual QString viewType() const;

	QAction* RefreshHostsAction;
	QAction* HostOnlineAction;
	QAction* HostOfflineAction;
	QAction* HostOfflineWhenDoneAction;
	QAction* HostRestartAction;
	QAction* HostRestartWhenDoneAction;
	QAction* HostRebootAction;
	QAction* HostRebootWhenDoneAction;
	QAction* HostShutdownAction;
	QAction* HostShutdownWhenDoneAction;
	QAction* HostMaintenanceEnableAction;

	QAction* RestartAction;
	QAction* VNCHostsAction;
	QAction* ClientUpdateAction;
	QAction* FilterAction;
	QAction* FilterClearAction;

	QAction* SubmitBatchJobAction;
	QAction* ShowHostInfoAction;
	QAction* ClearHostErrorsSetOfflineAction;
	QAction* ClearHostErrorsAction;
	QAction* ShowHostErrorsAction;
	QAction* ShowJobsAction;

	FilterEdit * mHostFilterEdit;

	virtual QToolBar * toolBar( QMainWindow * );
	virtual void populateViewMenu( QMenu * );

	RecordTreeView * hostTreeView() const;
	
public slots:

	void hostListSelectionChanged();
	
	void setHostsStatus(const QString & status);
	/// selected hosts are told to start burning
	void setHostsOnline();
	/// selected hosts are told to stop burning
	void setHostsOffline();
	void setHostsOfflineWhenDone();
	/// selected hosts are told to exit, and the process monitor is responsible for restarting Burner
	void setHostsRestart();
	void setHostsRestartWhenDone();
	void setHostsReboot();
	void setHostsRebootWhenDone();
	void setHostsShutdown();
	void setHostsShutdownWhenDone();

	// Enable maintenance mode
	void setHostsMaintenanceEnable();

	/// selected hosts are told to update to the latest Burner client
	void setHostsClientUpdate();

	void toggleFilter(bool);
	void clearFilters();

	/// opens vncviewer sessions to selected hosts
	void vncHosts();

	/// Opens a Job View and selects the jobs currently assigned to the selected hosts
	void showAssignedJobs();

	void showHostPopup(const QPoint &);

	void selectHosts( HostList );

	void applyOptions();

	void slotGroupingChanged(bool);
	
protected:
	/// refreshes the host list from the database
	void doRefresh();

	void save( IniConfig & ini, bool = false );
	void restore( IniConfig & ini, bool = false );

	void customEvent( QEvent * evt );

	RecordTreeView * mHostTree;

	bool mServiceDataRetrieved;
	ServiceList mServiceList, mServiceFilter;
	
	bool mHostTaskRunning;
	bool mQueuedHostRefresh;

	QToolBar * mToolBar;

	HostList mHostsToSelect;
	
	QString mStatsHTML;

public:
	QMenu * mHostMenu,
	 * mTailServiceLogMenu,
	 * mHostServiceFilterMenu,
	 * mCannedBatchJobMenu,
	 * mHostPluginMenu;

	friend class AssfreezerHostMenu;
	friend class AssFreezerWidget;
	friend class HostServiceFilterMenu;
	friend class CannedBatchJobMenu;
	friend class HostPluginMenu;
	friend class TailServiceLogMenu;
};

#endif // HOST_LIST_WIDGET_H
