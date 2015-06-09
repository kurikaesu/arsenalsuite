
#include <qlayout.h>
#include <qregexp.h>
#include <qmessagebox.h>
#include <qtoolbar.h>
#include <qwebview.h>
#include <qwebframe.h>

#include "path.h"
#include "database.h"
#include "freezercore.h"
#include "process.h"

#include "syslog.h"
#include "syslogrealm.h"
#include "syslogseverity.h"

#include "busywidget.h"
#include "filteredit.h"
#include "modelgrouper.h"
#include "recordtreeview.h"
#include "recordfilterwidget.h"
#include "viewcolors.h"

#include "afcommon.h"
#include "assfreezermenus.h"
#include "batchsubmitdialog.h"
#include "hostlistwidget.h"
#include "hostservice.h"
#include "hostselector.h"
#include "items.h"
#include "joblistwidget.h"
#include "mainwindow.h"
#include "remotetailwindow.h"
#include "remotetailwidget.h"
#include "threadtasks.h"

HostListWidget::HostListWidget( QWidget * parent )
: FreezerView( parent )
, mHostFilterEdit(0)
, mHostTree(0)
, mServiceDataRetrieved( false )
, mHostTaskRunning( false )
, mQueuedHostRefresh( false )
, mToolBar( 0 )
, mHostMenu( 0 )
, mTailServiceLogMenu( 0 )
, mHostServiceFilterMenu( 0 )
, mCannedBatchJobMenu( 0 )
{
	setupUi(this);

	RefreshHostsAction = new QAction( "Refresh Hosts", this );
	RefreshHostsAction->setIcon( QIcon( ":/images/refresh" ) );
	HostOnlineAction = new QAction( "Set Host(s) Status to Online", this );
	HostOnlineAction->setIcon( QIcon( ":/images/host_online" ) );
	HostOfflineAction = new QAction( "Set Host(s) Status to Offline", this );
	HostOfflineAction->setIcon( QIcon( ":/images/host_offline" ) );
	HostOfflineWhenDoneAction = new QAction( "Set Host(s) Status to Offline When Done", this );
	HostOfflineWhenDoneAction->setIcon( QIcon( ":/images/host_offline" ) );

	HostRestartAction = new QAction( "Restart Host(s) Now", this );
	HostRestartAction->setIcon( QIcon( ":/images/host_restart" ) );
	HostRestartWhenDoneAction = new QAction( "Restart Host(s) When Done", this );
	HostRestartWhenDoneAction->setIcon( QIcon( ":/images/host_restart" ) );

	HostRebootAction = new QAction( "Reboot Host(s) Now", this );
	HostRebootAction->setIcon( QIcon( ":/images/host_reboot" ) );
	HostRebootWhenDoneAction = new QAction( "Reboot Host(s) When Done", this );
	HostRebootWhenDoneAction->setIcon( QIcon( ":/images/host_reboot" ) );

	HostShutdownAction = new QAction( "Shutdown Host(s) Now", this );
	HostShutdownAction->setIcon( QIcon( ":/images/host_shutdown" ) );
	HostShutdownWhenDoneAction = new QAction( "Shutdown Host(s) When Done", this );
	HostShutdownWhenDoneAction->setIcon( QIcon( ":/images/host_shutdown" ) );

	HostMaintenanceEnableAction = new QAction( "Place Host(s) in Maintenance", this );
	HostMaintenanceEnableAction->setIcon( QIcon( ":/images/host_maintenance" ) );

	VNCHostsAction = new QAction( "VNC Hosts", this );
	VNCHostsAction->setIcon( QIcon( ":/images/vnc_hosts.png" ) );
	ClientUpdateAction = new QAction( "Client Update", this );
	ClientUpdateAction->setIcon( QIcon( ":/images/client_update.png" ) );

	FilterAction = new QAction( "&Filter", this );
	FilterAction->setIcon( QIcon( ":/images/filter" ) );
	FilterAction->setCheckable( TRUE );
	FilterAction->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_F ) );

	FilterClearAction = new QAction( "Clear Filters", this );
	FilterClearAction->setIcon( QIcon( ":/images/filterclear" ) );
	FilterClearAction->setShortcut( QKeySequence( Qt::CTRL + Qt::SHIFT + Qt::Key_F ) );

	SubmitBatchJobAction = new QAction( "Assign Batch Job", this );
	ShowHostInfoAction = new QAction( "Host Info...", this );
	ClearHostErrorsSetOfflineAction = new QAction( "Clear All Errors From Host and Set It Offline", this );
	ClearHostErrorsAction = new QAction( "Clear All Errors From Host", this );
	ShowHostErrorsAction = new QAction( "Show Host Errors", this );
	ShowJobsAction = new QAction( "Show Assigned Jobs", this );

	connect( RefreshHostsAction, SIGNAL( triggered(bool) ), SLOT( refresh() ) );
	connect( HostOnlineAction, SIGNAL( triggered(bool) ), SLOT( setHostsOnline() ) );
	connect( HostOfflineAction, SIGNAL( triggered(bool) ), SLOT( setHostsOffline() ) );
	connect( HostOfflineWhenDoneAction, SIGNAL( triggered(bool) ), SLOT( setHostsOfflineWhenDone() ) );
	connect( HostRestartAction, SIGNAL( triggered(bool) ), SLOT( setHostsRestart() ) );
	connect( HostRestartWhenDoneAction, SIGNAL( triggered(bool) ), SLOT( setHostsRestartWhenDone() ) );
	connect( HostRebootAction, SIGNAL( triggered(bool) ), SLOT( setHostsReboot() ) );
	connect( HostRebootWhenDoneAction, SIGNAL( triggered(bool) ), SLOT( setHostsRebootWhenDone() ) );
	connect( HostShutdownAction, SIGNAL( triggered(bool) ), SLOT( setHostsShutdown() ) );
	connect( HostShutdownWhenDoneAction, SIGNAL( triggered(bool) ), SLOT(setHostsShutdownWhenDone() ) );
	connect( HostMaintenanceEnableAction, SIGNAL( triggered(bool) ), SLOT(setHostsMaintenanceEnable() ) );

	connect( ClientUpdateAction, SIGNAL( triggered(bool) ), SLOT( setHostsClientUpdate() ) );
	connect( VNCHostsAction, SIGNAL( triggered(bool) ), SLOT( vncHosts() ) );
	connect( ShowJobsAction, SIGNAL(triggered(bool)), SLOT( showAssignedJobs() ) );
	connect( FilterAction, SIGNAL( triggered(bool) ), SLOT( toggleFilter(bool) ) );
	connect( FilterClearAction, SIGNAL( triggered(bool) ), SLOT( clearFilters() ) );

	mHostFilterEdit = new FilterEdit( this, Host::c.Name, "Host Name Filter:" );
	connect( mHostFilterEdit, SIGNAL(filterChanged(const Expression &)), SLOT(refresh()) );

	mHostTree = new RecordTreeView(this);
	QLayout * hbox = new QHBoxLayout(this);
	hbox->setMargin(0);
	hbox->addWidget(mHostTree);

	/* ListView connections */
	connect( mHostTree, SIGNAL( selectionChanged(RecordList) ), SLOT( hostListSelectionChanged() ) );

	mHostTree->setContextMenuPolicy( Qt::CustomContextMenu );
	connect( mHostTree, SIGNAL( customContextMenuRequested( const QPoint & ) ), SLOT( showHostPopup( const QPoint & ) ) );
	//connect( mHostTree, SIGNAL( itemDoubleClicked( QTreeWidgetItem *, int ) ), SLOT( vncHosts() ) );

	RecordSuperModel * hm = new RecordSuperModel(mHostTree);
	new HostTranslator( hm->treeBuilder() );
	// Enable grouping
	hm->grouper()->setColumnGroupRegex( 0, QRegExp( "[^\\d]+" ) );
	connect( hm->grouper(), SIGNAL( groupingChanged(bool) ), SLOT( slotGroupingChanged(bool) ) );
	
	hm->setAutoSort( true );
	mHostTree->setModel( hm );

	mTailServiceLogMenu = new TailServiceLogMenu( this );
	mHostServiceFilterMenu = new HostServiceFilterMenu( this );
	mCannedBatchJobMenu = new CannedBatchJobMenu( this );
    mHostPluginMenu = new HostPluginMenu( this );

	// Set defaults
	IniConfig temp;
	restore(temp);

	IniConfig & ini = viewConfig();
	FilterAction->setChecked( ini.readBool( "Filter", true ) );
	{
		JobModel * jm = new JobModel( mJobTree );
		jm->setAutoSort( false );
		mJobTree->setModel( jm );
		mJobTree->setItemDelegateForColumn( 2, new ProgressDelegate( mJobTree ) );
		mJobTree->setDragEnabled( false );
		mJobTree->setAcceptDrops( false );
		mJobTree->setDropIndicatorShown(false);
		setupJobView(mJobTree, ini);
		mJobTree->setFont( options.jobFont );
		options.mJobColors->apply(mJobTree);

		QPalette p = mJobTree->palette();
		ColorOption * co = options.mJobColors->getColorOption("default");
		p.setColor(QPalette::Active, QPalette::AlternateBase, co->bg.darker(120));
		p.setColor(QPalette::Inactive, QPalette::AlternateBase, co->bg.darker(120));
		mJobTree->setPalette( p );

		mJobTree->enableFilterWidget(false);
		mJobTree->update();
	}

	FreezerCore::addTask( new StaticHostListDataTask( this ) );

	mStatsHTML = readFullFile("resources/ganglia/index.html");
}

HostListWidget::~HostListWidget()
{
}

QString HostListWidget::viewType() const
{
	return "HostList";
}

RecordTreeView * HostListWidget::hostTreeView() const
{
	return mHostTree;
}

void HostListWidget::save( IniConfig & ini, bool forceFullSave )
{
	ini.writeString("ViewType","HostList");
	ini.writeString("ServiceFilter", mServiceFilter.keyString());
	ini.writeBool( "Filter", FilterAction->isChecked() );
	QStringList sizes;
	foreach( int i, mHostSplitter->sizes() ) sizes += QString::number(i);
		ini.writeString( "HostSplitterPos", sizes.join(",") );

	saveHostView(mHostTree,ini);
	saveJobView(mJobTree,ini);
	FreezerView::save(ini);
}

void HostListWidget::restore( IniConfig & ini, bool forceFullRestore )
{
	setupHostView(mHostTree,ini);
	QStringList frameSplitterSizes = ini.readString( "HostSplitterPos","600,100,400" ).split(',');
	QList<int> frameSplitterInts;
	for( QStringList::Iterator it=frameSplitterSizes.begin(); it!=frameSplitterSizes.end(); ++it )
		frameSplitterInts += (*it).toInt();
	mHostSplitter->setSizes( frameSplitterInts );

	FreezerView::restore(ini,forceFullRestore);
}

void HostListWidget::customEvent( QEvent * evt )
{
	if( ((ThreadTask*)evt)->mCancel )
		return;
		
	switch( (int)evt->type() ) {
		case HOST_LIST:
		{
			LOG_5( "Updating host view items" );
			QTime t;
			t.start();
			HostList updated = ((HostListTask*)evt)->mReturn;
			LOG_5( "updated "+ QString::number(updated.size()) + " hosts from db" );
			// Recursive so the update works with grouping
			mHostTree->model()->updateRecords( updated, QModelIndex(), /*recursive=*/true );

			LOG_5( "Enabling updates took " + QString::number( t.elapsed() ) + " ms" );

			if( mHostsToSelect.size() ) {
				mHostTree->setSelection( mHostsToSelect );
				mHostTree->sortBySelection();
				mHostTree->scrollTo( mHostsToSelect );
				mHostsToSelect = HostList();
			}

			mHostTree->busyWidget()->stop();
			
			mHostTaskRunning = false;
			if( mQueuedHostRefresh ) {
				mQueuedHostRefresh = false;
				refresh();
			} else
				clearStatusBar();

			mHostTree->mRecordFilterWidget->filterRows();

			break;
		}
		case STATIC_HOST_LIST_DATA:
		{
			mServiceDataRetrieved = true;
			StaticHostListDataTask * sdt = (StaticHostListDataTask*)evt;
			mServiceList = sdt->mServices;
			IniConfig & ini = viewConfig();
			// Show All Services by default
			mServiceFilter = Service::table()->records( ini.readUIntList("ServiceFilter", mServiceList.keys()) );
			if( mQueuedHostRefresh ) {
				mQueuedHostRefresh = false;
				doRefresh();
			}
			mHostTree->mRecordFilterWidget->filterRows();
			break;
		}
		case UPDATE_HOST_LIST:
		{
			HostList hl = ((UpdateHostListTask*)evt)->mReturn;
			mHostTree->model()->updated(hl);
			refresh();
			mHostTree->mRecordFilterWidget->filterRows();
			break;
		}
		default:
			break;
	}
}

void HostListWidget::doRefresh()
{
	FreezerView::doRefresh();
	bool needStatusBarMsg = false, needHostListTask = false;

	if( !mServiceDataRetrieved )
		needStatusBarMsg = mQueuedHostRefresh = true;
	else if( mHostTaskRunning )
		mQueuedHostRefresh = true;
	else
		needStatusBarMsg = needHostListTask = true;

	if( needStatusBarMsg )
		setStatusBarMessage( "Refreshing Host List..." );

	if( needHostListTask ) {
		mHostTaskRunning = true;
		mHostTree->busyWidget()->start();
		FreezerCore::addTask( new HostListTask( this, mServiceFilter, mServiceList, mHostFilterEdit->expression(), !mHostTree->isColumnHidden(15) /*Services Column*/, !mHostTree->isColumnHidden(17) /*IP Address*/ ) );
		FreezerCore::wakeup();
	}

	toggleFilter( FilterAction->isChecked() );
}

void HostListWidget::hostListSelectionChanged()
{
	HostList hosts = mHostTree->selection();
	if( hosts.size() == 1 ) {
		setStatusBarMessage( hosts[0].name() + " Selected" );
	} else if( hosts.size() )
		setStatusBarMessage( QString::number( hosts.size() ) + " Hosts Selected" );
	else {
		clearStatusBar();
		return;
	}

	QString gangliaGroup = "Workstations";
	if( hosts[0].name().startsWith("c0") )
		gangliaGroup = "Blades";

	QString loadimg = QString("http://ganglia/graph.php?g=load_report&z=medium&c=%2&h=%1.drd.int&m=load_one&r=hour&s=descending&hc=4&mc=2").arg(hosts[0].name()).arg(gangliaGroup);
	QString memoryimg = QString("http://ganglia/graph.php?g=mem_report&z=medium&c=%2&h=%1.drd.int&m=load_one&r=hour&s=descending&hc=4&mc=2").arg(hosts[0].name()).arg(gangliaGroup);
	QString cpuimg = QString("http://ganglia/graph.php?g=cpu_report&z=medium&c=%2&h=%1.drd.int&m=load_one&r=hour&s=descending&hc=4&mc=2").arg(hosts[0].name()).arg(gangliaGroup);
	QString networkimg = QString("http://ganglia/graph.php?g=network_report&z=medium&c=%2&h=%1.drd.int&m=load_one&r=hour&s=descending&hc=4&mc=2").arg(hosts[0].name()).arg(gangliaGroup);
	QString nfsimg = QString("http://ganglia/graph.php?g=nfs_report&z=medium&c=%2&h=%1.drd.int&m=load_one&r=hour&s=descending&hc=4&mc=2").arg(hosts[0].name()).arg(gangliaGroup);

	QString html = mStatsHTML;
	html.replace("%LOADAVGIMG%", loadimg);
	html.replace("%MEMORYIMG%", memoryimg);
	html.replace("%CPUIMG%", cpuimg);
	html.replace("%NETWORKIMG%", networkimg);
	html.replace("%NFSIMG%", nfsimg);

	mStatsView->setHtml(html);

	if( mHostTree->selection().size() < 10 )
		mJobTree->model()->updateRecords(Host::activeAssignments(mHostTree->selection()).jobs());
}

void HostListWidget::selectHosts( HostList hosts )
{
	mHostTree->setSelection( hosts );
	mHostTree->scrollTo( hosts );
	if( mHostTaskRunning || mQueuedHostRefresh || refreshCount() == 0 )
		mHostsToSelect = hosts;
}

QToolBar * HostListWidget::toolBar( QMainWindow * mw )
{
	if( !mToolBar ) {
		mToolBar = new QToolBar( mw );
		mToolBar->addAction( RefreshHostsAction );
		mToolBar->addSeparator();
		mToolBar->addAction( HostOnlineAction );
		mToolBar->addAction( HostOfflineAction );
		mToolBar->addAction( HostRestartAction );
		mToolBar->addAction( VNCHostsAction );
		mToolBar->addSeparator();
		mToolBar->addAction( FilterAction );
		mToolBar->addAction( FilterClearAction );
		mToolBar->addWidget( mHostFilterEdit );
	}
	return mToolBar;
}

void HostListWidget::populateViewMenu( QMenu * viewMenu )
{
	viewMenu->addMenu( mHostServiceFilterMenu );
	viewMenu->addSeparator();
	viewMenu->addAction( FilterAction );
	viewMenu->addAction( FilterClearAction );
}

void HostListWidget::setHostsStatus(const QString & status)
{
	// Get a list of ids for the selected hosts
	HostList hosts = mHostTree->selection();
	if( hosts.size()==0 )
		return;

	foreach( Host h, hosts )
		FreezerCore::addTask( new UpdateHostListTask( this, HostList(h), status ) );
	FreezerCore::wakeup();

	HostStatusList hsl = hosts.hostStatuses();
	hsl.setSlaveStatuses(status);

	SysLog log;
	log.setSysLogRealm( SysLogRealm::recordByName("Farm") );
	log.setSysLogSeverity( SysLogSeverity::recordByName("Warning") );
	log.setMessage( QString("%1 hosts set to %2").arg(hosts.size()).arg(status) );
	log.set_class("HostListWidget");
	log.setMethod("setHostsStatus");
	log.setUserName( User::currentUser().name() );
	log.setHostName( Host::currentHost().name() );
	log.commit();
}

void HostListWidget::setHostsOnline()
{
	setHostsStatus("starting");
	refresh();
}

void HostListWidget::setHostsOffline()
{
	setHostsStatus("stopping");
	refresh();
}

void HostListWidget::setHostsOfflineWhenDone()
{
	HostList hosts = mHostTree->selection();
	if( hosts.size()==0 )
		return;

	Database::current()->exec( "UPDATE HostStatus SET slaveStatus = 'offline-when-done' WHERE fkeyHost IN(" +hosts.keyString() + ");" );
}

void HostListWidget::setHostsRestart()
{
	HostList hosts = mHostTree->selection();
	if( hosts.hostStatuses().filter("status","busy").size() > 0 ) {
		QString hostNames = hosts.names().join(",");
		if( hostNames.size() > 100 )
			hostNames = QString::number(hosts.size()) + " hosts [" + hostNames.left(80) + "]";
		if( QMessageBox::warning( this, "Restart Hosts", "Are you sure that you want to restart the following hosts?\n" + hostNames + "\nSome of the hosts are busy and the current progress will be lost.",
				QMessageBox::Yes, QMessageBox::Cancel ) != QMessageBox::Yes )
			return;
	}
	setHostsStatus("restart");
	refresh();
}

void HostListWidget::setHostsRestartWhenDone()
{
	HostList hosts = mHostTree->selection();
	if( hosts.size()==0 )
		return;

	Database::current()->exec( "UPDATE HostStatus SET slaveStatus = 'restart-when-done' WHERE fkeyHost IN(" + hosts.keyString() + ");" );
	refresh();
}

void HostListWidget::setHostsReboot()
{
	setHostsStatus("reboot");
	refresh();
}

void HostListWidget::setHostsRebootWhenDone()
{
	HostList hosts = mHostTree->selection();
	if( hosts.size()==0 )
		return;

	Database::current()->exec( "UPDATE HostStatus SET slaveStatus = 'reboot-when-done' WHERE fkeyHost IN(" + hosts.keyString() + ");" );
	refresh();
}

void HostListWidget::setHostsShutdown()
{
	setHostsStatus("shutdown");
}

void HostListWidget::setHostsShutdownWhenDone()
{
	HostList hosts = mHostTree->selection();
	if( hosts.size()==0 )
		return;

	Database::current()->exec( "UPDATE HostStatus SET slaveStatus = 'shutdown-when-done' WHERE fkeyHost IN(" + hosts.keyString() + ");" );
}

void HostListWidget::setHostsMaintenanceEnable()
{
	setHostsStatus("maintenance");
}

void HostListWidget::setHostsClientUpdate()
{
	setHostsStatus("client-update");
	refresh();
}

void HostListWidget::vncHosts()
{
	foreach( Host h, mHostTree->selection() )
		vncHost( h.name() );
}

void HostListWidget::showAssignedJobs()
{
	MainWindow * mw = qobject_cast<MainWindow*>(window());
	if( mw ) {
		JobListWidget * jobList = new JobListWidget(mw);
		jobList->setJobList( Host::activeAssignments(mHostTree->selection()).jobs() );
		mw->insertView(jobList);
		mw->setCurrentView(jobList);
	}
}

void HostListWidget::applyOptions()
{
	options.mHostColors->apply(mHostTree);
	mHostTree->setFont( options.jobFont );

	QPalette p = mHostTree->palette();
	ColorOption * co = options.mHostColors->getColorOption("default");
	p.setColor(QPalette::Active, QPalette::AlternateBase, co->bg.darker(120));
	p.setColor(QPalette::Inactive, QPalette::AlternateBase, co->bg.darker(120));
	mHostTree->setPalette( p );

	mHostTree->update();
}

void HostListWidget::slotGroupingChanged(bool grouped)
{
	mHostTree->setRootIsDecorated(grouped);
}

QString jobErrorAsString( const JobError & je )
{
	QDateTime dt;
	dt.setTime_t(je.errorTime());
	return "[" + je.host().name() + "; " + dt.toString() + "; Frames " + je.frames() + "]\n" + je.message();
}

QString verifyKeyList( const QString & list, Table * table )
{
	QList<uint> keys;
	QStringList sl = list.split(',');
	foreach( QString key, sl )
		keys += key.toInt();
	return table->records(keys).keyString();
}

QList<uint> verifyKeyList( const QList<uint> & keys, Table * table )
{
	return table->records(keys).keys();
}

void clearHostErrorsAndSetOffline( HostList hosts, bool offline)
{
	if( hosts.isEmpty() ) return;

	if (offline) {
		QString subject = "Setting Hosts Offline for repair: " + hosts.names().join(",");
		QString body = subject + "\\nAll Current Errors have been cleared";
		foreach( Host h, hosts ) {
			body += h.name() + "\\n";
			JobErrorList errors = JobError::select("fkeyhost=? order by errortime desc limit 10", VarList() << h.key());
			if( !errors.isEmpty() ) {
				body += "\\n";
				foreach(JobError je, errors)
					body += jobErrorAsString(je).replace("\n","\\n").replace("'","") + "\\n";
			}
			body += "\\n\\n";
		}

		QString from = User::currentUser().name() + "@blur.com";
		sendEmail( QStringList() << "it@blur.com", subject, body, from );
	}

	Database::current()->beginTransaction();
	Database::current()->exec( "UPDATE JobError SET cleared=true WHERE fkeyhost IN (" + hosts.keyString() + ")" );
	if (offline) {
		HostStatusList hsl = hosts.hostStatuses();
		hsl.setSlaveStatuses( "offline" );
		hsl.commit();
	}
	Database::current()->commitTransaction();
}

void HostListWidget::showHostPopup(const QPoint & point)
{
	if( !mHostMenu ) mHostMenu = new FreezerHostMenu( this );
	mHostMenu->popup( mHostTree->mapToGlobal(point) );
}

void HostListWidget::toggleFilter(bool enable)
{
	mHostTree->enableFilterWidget(enable);
}

void HostListWidget::clearFilters()
{
	mHostTree->mRecordFilterWidget->clearFilters();
}
