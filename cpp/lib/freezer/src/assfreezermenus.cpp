

#include <qclipboard.h>
#include <qfileinfo.h>
#include <qinputdialog.h>
#include <qmessagebox.h>
#include <qprocess.h>

#include "database.h"
#include "path.h"
#include "process.h"

#include "group.h"
#include "host.h"
#include "hostlistwidget.h"
#include "hostservice.h"
#include "jobassignment.h"
#include "jobassignmentstatus.h"
#include "jobcannedbatch.h"
#include "jobbatch.h"
#include "jobdep.h"
#include "jobhistory.h"
#include "jobhistorytype.h"
#include "jobtaskassignment.h"
#include "user.h"
#include "usergroup.h"

#include "hostdialog.h"
#include "hosthistoryview.h"
#include "hostselector.h"
#include "recordpropvaltree.h"
#include "remotetailwindow.h"

#include "jobassignmentwindow.h"

#include "assfreezermenus.h"
#include "batchsubmitdialog.h"
#include "errorlistwidget.h"
#include "hosterrorwindow.h"
#include "items.h"
#include "joblistwidget.h"
#include "mainwindow.h"

#include "jobviewerfactory.h"
#include "jobviewerplugin.h"
#include "hostviewerfactory.h"
#include "hostviewerplugin.h"
#include "frameviewerfactory.h"
#include "frameviewerplugin.h"
#include "multiframeviewerfactory.h"
#include "multiframeviewerplugin.h"

static FreezerMenuFactory * sMenuFactory = 0;

FreezerMenuFactory * FreezerMenuFactory::instance()
{
	if( !sMenuFactory )
		sMenuFactory = new FreezerMenuFactory();
	return sMenuFactory;
}
	
void FreezerMenuFactory::registerMenuPlugin( FreezerMenuPlugin * plugin, const QString & menuName )
{
	mPlugins[menuName].append(plugin);
}

QList<QAction*> FreezerMenuFactory::aboutToShow( QMenu * menu, bool , bool )
{
	QList<QAction*> before = menu->actions(), ret;
	QString name = menu->objectName();
	if( name.isEmpty() )
		name = menu->metaObject()->className();
	if( mPlugins.contains( menu->objectName() ) )
		foreach( FreezerMenuPlugin * plugin, mPlugins[menu->objectName()] )
			plugin->executeMenuPlugin( menu );
	foreach( QAction * action, menu->actions() )
		if( !before.contains( action ) )
			ret.append(action);
	return ret;
}

FreezerMenu::FreezerMenu( QWidget * parent, const QString & title)
: QMenu( title, parent )
{
	connect( this, SIGNAL( aboutToShow() ), SLOT( slotAboutToShow() ) );
	connect( this, SIGNAL( triggered( QAction * ) ), SLOT( slotActionTriggered( QAction * ) ) );
	MainWindow * mw = qobject_cast<MainWindow*>(parent->window());
	if( mw ) {
		connect( mw, SIGNAL( currentViewChanged( FreezerView * ) ), SLOT( slotCurrentViewChanged( FreezerView * ) ), Qt::QueuedConnection );
	} else
		LOG_1( "Unable to cast window to MainWindow pointer, window is of type: " + QString(parent->window()->metaObject()->className()) );
}

void FreezerMenu::slotCurrentViewChanged( FreezerView * view )
{
	if( view && isTearOffMenuVisible() )
		slotAboutToShow();
	setEnabled( view );
}

template<class BASE> void JobListMenu<BASE>::slotCurrentViewChanged( FreezerView * view )
{
	JobListWidget * jobList = qobject_cast<JobListWidget*>(view);
	if( jobList )
		mJobList = jobList;
	FreezerMenu::slotCurrentViewChanged( jobList );
}

template<class BASE> void HostListMenu<BASE>::slotCurrentViewChanged( FreezerView * view )
{
	HostListWidget * hostList = qobject_cast<HostListWidget*>(view);
	if( hostList )
		mHostList = hostList;
	FreezerMenu::slotCurrentViewChanged( hostList );
}

template<class BASE> void ErrorListMenu<BASE>::slotCurrentViewChanged( FreezerView * view )
{
	ErrorListWidget * errorList = qobject_cast<ErrorListWidget*>(view);
	if( errorList )
		mErrorList = errorList;
	FreezerMenu::slotCurrentViewChanged( errorList );
}

ProjectFilterMenu::ProjectFilterMenu(JobListWidget * jobList)
: JobListMenu<>( jobList, "Project Filter" )
, mProjectShowAll( 0 )
, mProjectShowNone( 0 )
, mProjectShowNonProject( 0 )
, mProjectActionsCreated( false )
{
	setTearOffEnabled( true );
}

void ProjectFilterMenu::slotAboutToShow()
{
	if( !mProjectActionsCreated ) {
		mProjectActionsCreated = true;
		mProjectShowAll = new QAction( "Show All", this );
		mProjectShowAll->setCheckable( true );
		mProjectShowNone = new QAction( "Show None", this );
		mProjectShowNone->setCheckable( true );
		mProjectShowNonProject = new QAction( "Non-Project Jobs", this );
		mProjectShowNonProject->setCheckable( true );
		ProjectList projectList = mJobList->activeProjects().sorted( "name" );
		foreach( Project p, projectList ) {
			QAction * act = new QAction( p.name(), this );
			act->setCheckable( true );
			act->setProperty( "projectkey", QVariant(p.key()) );
			mProjectActions.append( act );
		}
		addAction( mProjectShowAll );
		addAction( mProjectShowNone );
		addSeparator();
		addAction( mProjectShowNonProject );
		addActions( mProjectActions );
	}
	updateActionStates();
	FreezerMenuFactory::instance()->aboutToShow( this, true, true );
}

void ProjectFilterMenu::slotActionTriggered( QAction * act )
{
	if( act == mProjectShowAll ) {
		mJobList->mJobFilter.allProjectsShown = true;
		mJobList->mJobFilter.showNonProjectJobs = true;
		mJobList->mJobFilter.visibleProjects = mJobList->activeProjects().keys();
	} else if( act == mProjectShowNone ) {
		mJobList->mJobFilter.visibleProjects.clear();
		mJobList->mJobFilter.allProjectsShown = false;
		mJobList->mJobFilter.showNonProjectJobs = false;
	} else if( act == mProjectShowNonProject ) {
		mJobList->mJobFilter.showNonProjectJobs = act->isChecked();
		mJobList->mJobFilter.allProjectsShown = false;
	} else {
		bool show = act->isChecked();
		if( !show && mJobList->mJobFilter.allProjectsShown ) {
			mJobList->mJobFilter.allProjectsShown = false;
			mJobList->mJobFilter.visibleProjects = mJobList->activeProjects().keys();
		}
		int projectKey = act->property("projectkey").toInt();
		int i = mJobList->mJobFilter.visibleProjects.indexOf( projectKey );
		if( i < 0 )
			mJobList->mJobFilter.visibleProjects += projectKey;
		else
			mJobList->mJobFilter.visibleProjects.removeAt(i);
		if( mJobList->mJobFilter.visibleProjects.size() == int(mJobList->activeProjects().size()) )
			mJobList->mJobFilter.allProjectsShown = true;
	}
	updateActionStates();
	mJobList->refresh();
}

void ProjectFilterMenu::updateActionStates()
{
	mProjectShowAll->setChecked( mJobList->mJobFilter.allProjectsShown && mJobList->mJobFilter.showNonProjectJobs );
	mProjectShowAll->setEnabled( !mProjectShowAll->isChecked() );
	mProjectShowNone->setChecked( !mJobList->mJobFilter.allProjectsShown && !mJobList->mJobFilter.showNonProjectJobs && mJobList->mJobFilter.visibleProjects.isEmpty() );
	mProjectShowNone->setCheckable( !mProjectShowNone->isChecked() );
	mProjectShowNonProject->setChecked( mJobList->mJobFilter.showNonProjectJobs );
	foreach( QAction * act, mProjectActions )
		act->setChecked( mJobList->mJobFilter.allProjectsShown || mJobList->mJobFilter.visibleProjects.contains( act->property("projectkey").toInt() ) );
}

StatusFilterMenu::StatusFilterMenu(JobListWidget * jobList)
: JobListMenu<>( jobList, "Status Filter" )
, mStatusActionsCreated( false )
, mStatusShowAll( 0 )
, mStatusShowNone( 0 )
{
	setTearOffEnabled( true );
	setWindowTitle( "Status Filter" );
}

const char * stats[] = { "Submit", "Verify", "Ready", "Holding", "Started", "Suspended", "Done", "Archived", "Deleted", 0 };

void StatusFilterMenu::slotAboutToShow()
{
	if( !mStatusActionsCreated )
	{
		mStatusActionsCreated = true;
		mStatusShowAll = new QAction( "Show All", this );
		mStatusShowAll->setCheckable( true );
		mStatusShowNone = new QAction( "Show None", this );
		mStatusShowNone->setCheckable( true );
		for(int i=0;stats[i]; i++){
			QString stat(stats[i]);
			QAction * act = new QAction( stat, this );
			act->setCheckable( true );
			if( mJobList->mJobFilter.statusToShow.contains(stat.toLower()) )
				act->setChecked(true);
			mStatusActions.append( act );
		}
		addAction( mStatusShowAll );
		addAction( mStatusShowNone );
		addSeparator();
		addActions( mStatusActions );
	}
	updateActionStates();
	FreezerMenuFactory::instance()->aboutToShow( this, true, true );
}

void StatusFilterMenu::updateActionStates()
{
	mStatusShowAll->setChecked( mJobList->mJobFilter.statusToShow.size() == mStatusActions.size() );
	mStatusShowAll->setEnabled( !mStatusShowAll->isChecked() );
	mStatusShowNone->setChecked( mJobList->mJobFilter.statusToShow.size() == 0 );
	mStatusShowNone->setEnabled( !mStatusShowNone->isChecked() );
	foreach( QAction * act, mStatusActions )
		act->setChecked( mJobList->mJobFilter.statusToShow.contains( act->text().toLower() ) );
}

void StatusFilterMenu::slotActionTriggered( QAction * act )
{
	if( act == mStatusShowAll ) {
		mJobList->mJobFilter.statusToShow.clear();
		for( int i=0; stats[i]; i++ )
			mJobList->mJobFilter.statusToShow += QString(stats[i]).toLower();
	} else if( act == mStatusShowNone ) {
		mJobList->mJobFilter.statusToShow.clear();
	} else {
		int i = 0;
		for( ; stats[i]; i++ )
			if( stats[i] == act->text() )
				break;
		if( stats[i] ) {
			QString stat = QString( stats[i] ).toLower();
			int i = mJobList->mJobFilter.statusToShow.indexOf( stat );
			LOG_5( "Status " + stat + " changed to " + (act->isChecked() ? "true" : "false") + " pos is " + QString::number(i) );
			if( i >= 0 )
				mJobList->mJobFilter.statusToShow.removeAt( i );
			else
				mJobList->mJobFilter.statusToShow += stat;
		}
	}
	updateActionStates();
	mJobList->refresh();
}

JobTypeFilterMenu::JobTypeFilterMenu(FreezerView * view, const QString & )
: FreezerMenu( view, "Job Type Filter" )
, mJobTypeActionsCreated( false )
, mJobTypeShowAll( 0 )
, mJobTypeShowNone( 0 )
{
	setTearOffEnabled( true );
}

void JobTypeFilterMenu::slotAboutToShow()
{
	if( !mJobTypeActionsCreated ) {
		mJobTypeActionsCreated = true;

		mJobTypeShowAll = new QAction( "Show All", this );
		mJobTypeShowAll->setCheckable( true );
		mJobTypeShowNone = new QAction( "Show None", this );
		mJobTypeShowNone->setCheckable( true );
		JobTypeList jtl = activeJobTypes().sorted( "jobType" );
		foreach( JobType jt, jtl )
		{
			// For now skip filtering of sub-jobtypes
			if( jt.parentJobType().isRecord() ) continue;

			QImage img = jt.icon();
			QAction * act = new QAction( img.isNull() ? QIcon() : QPixmap::fromImage(jt.icon()), jt.name(), this );
			act->setCheckable( true );
			act->setProperty( "jobtypekey", QVariant(jt.key()) );
			mJobTypeActions.append( act );
		}
		
		addAction( mJobTypeShowAll );
		addAction( mJobTypeShowNone );
		addSeparator();
		addActions( mJobTypeActions );
	}
	updateActionStates();
	FreezerMenuFactory::instance()->aboutToShow( this, true, true );
}

void JobTypeFilterMenu::updateActionStates()
{
	JobTypeList filter = jobTypeFilter(), active = activeJobTypes();
	mJobTypeShowAll->setChecked( filter.size() == active.size() );
	mJobTypeShowAll->setEnabled( !mJobTypeShowAll->isChecked() );
	mJobTypeShowNone->setChecked( filter.isEmpty() );
	mJobTypeShowNone->setEnabled( !mJobTypeShowNone->isChecked() );
	foreach( QAction * act, mJobTypeActions ) {
		act->setChecked( filter.contains(JobType(act->property("jobtypekey").toInt())) );
	}
}

void JobTypeFilterMenu::slotActionTriggered( QAction * act )
{
	if( act == mJobTypeShowAll ) {
		setJobTypeFilter( activeJobTypes().filter( "fkeyparentjobtype", QVariant(QVariant::Int) /*NULL*/ ) );
	} else if( act == mJobTypeShowNone ) {
		setJobTypeFilter( JobTypeList() );
	} else {
		JobTypeList filter = jobTypeFilter();
		JobType jt(act->property("jobtypekey").toInt());
		if( filter.contains(jt) )
			filter.remove(jt);
		else
			filter += jt;
		setJobTypeFilter(filter);
	}
	updateActionStates();
}

JobListJobTypeFilterMenu::JobListJobTypeFilterMenu(JobListWidget * jobList)
: JobListMenu<JobTypeFilterMenu>( jobList )
{}

JobTypeList JobListJobTypeFilterMenu::jobTypeFilter() const
{
	return JobType::table()->records( mJobList->mJobFilter.typesToShow );
}

void JobListJobTypeFilterMenu::setJobTypeFilter( JobTypeList filter )
{
	mJobList->mJobFilter.typesToShow = filter.keys();
	mJobList->refresh();
}

JobTypeList JobListJobTypeFilterMenu::activeJobTypes() const
{
	return mJobList->activeJobTypes();
}

ErrorListJobTypeFilterMenu::ErrorListJobTypeFilterMenu(ErrorListWidget * errorList)
: ErrorListMenu<JobTypeFilterMenu>( errorList )
{}

JobTypeList ErrorListJobTypeFilterMenu::jobTypeFilter() const
{
	return mErrorList->mJobTypeFilter;
}

void ErrorListJobTypeFilterMenu::setJobTypeFilter( JobTypeList filter )
{
	mErrorList->mJobTypeFilter = filter;
	mErrorList->refresh();
}

JobTypeList ErrorListJobTypeFilterMenu::activeJobTypes() const
{
	return JobType::select();
}

ServiceFilterMenu::ServiceFilterMenu(FreezerView * view, const QString & title)
: FreezerMenu( view, title )
, mJobServiceActionsCreated( false )
, mJobServiceShowAll( 0 )
, mJobServiceShowNone( 0 )
{
	setObjectName( "Freezer_JobServiceMenu" );
	setTearOffEnabled( true );
}

void ServiceFilterMenu::slotAboutToShow()
{
	if( !mJobServiceActionsCreated ) {
		mJobServiceActionsCreated = true;

		mJobServiceShowAll = new QAction( "Show All", this );
		mJobServiceShowAll->setCheckable( true );
		mJobServiceShowNone = new QAction( "Show None", this );
		mJobServiceShowNone->setCheckable( true );
		ServiceList sl = activeServices().sorted( "service" );
		foreach( Service s, sl )
		{
			QAction * act = new QAction( s.service(), this );
			act->setCheckable( true );
			act->setProperty( "jobservicekey", QVariant(s.key()) );
			mJobServiceActions.append( act );
		}
		
		addAction( mJobServiceShowAll );
		addAction( mJobServiceShowNone );
		addSeparator();
		addActions( mJobServiceActions );
	}
	updateActionStates();
	FreezerMenuFactory::instance()->aboutToShow( this, true, true );
}

void ServiceFilterMenu::updateActionStates()
{
	ServiceList filter = serviceFilter(), active = activeServices();
	mJobServiceShowAll->setChecked( filter.size() == active.size() );
	mJobServiceShowAll->setEnabled( !mJobServiceShowAll->isChecked() );
	mJobServiceShowNone->setChecked( filter.size() == 0 );
	mJobServiceShowNone->setEnabled( !mJobServiceShowNone->isChecked() );
	foreach( QAction * act, mJobServiceActions )
		act->setChecked( filter.contains(Service(act->property("jobservicekey").toInt())) );
}

void ServiceFilterMenu::slotActionTriggered( QAction * act )
{
	if( act == mJobServiceShowAll ) {
		setServiceFilter(activeServices());
	} else if( act == mJobServiceShowNone ) {
		setServiceFilter(ServiceList());
	} else {
		Service s(act->property("jobservicekey").toInt());
		ServiceList filter = serviceFilter();
		if( filter.contains(s) )
			filter.remove(s);
		else
			filter += s;
		setServiceFilter(filter);
	}
	updateActionStates();
}

JobServiceFilterMenu::JobServiceFilterMenu(JobListWidget * jobList)
: JobListMenu<ServiceFilterMenu>( jobList, "Job Service Filter" )
{}

ServiceList JobServiceFilterMenu::serviceFilter() const
{
	return Service::table()->records(mJobList->mJobFilter.servicesToShow);
}

void JobServiceFilterMenu::setServiceFilter( ServiceList filter )
{
	mJobList->mJobFilter.servicesToShow = filter.keys();
	mJobList->refresh();
}

ServiceList JobServiceFilterMenu::activeServices() const
{
	return mJobList->activeServices();
}

HostServiceFilterMenu::HostServiceFilterMenu(HostListWidget * hostList)
: HostListMenu<ServiceFilterMenu>( hostList, "Host Service Filter" )
{}

ServiceList HostServiceFilterMenu::serviceFilter() const
{
	return mHostList->mServiceFilter;
}

void HostServiceFilterMenu::setServiceFilter( ServiceList filter )
{
	mHostList->mServiceFilter = filter;
	mHostList->refresh();
}

ServiceList HostServiceFilterMenu::activeServices() const
{
	return mHostList->mServiceList;
}

ErrorServiceFilterMenu::ErrorServiceFilterMenu(ErrorListWidget * errorList)
: ErrorListMenu<ServiceFilterMenu>( errorList, "Error Service Filter" )
{}

ServiceList ErrorServiceFilterMenu::serviceFilter() const
{
	return mErrorList->mServiceFilter;
}

ServiceList ErrorServiceFilterMenu::activeServices() const
{
	return Service::select();
}

void ErrorServiceFilterMenu::setServiceFilter( ServiceList filter )
{
	mErrorList->mServiceFilter = filter;
	mErrorList->refresh();
}

FreezerJobMenu::FreezerJobMenu(JobListWidget * jobList)
: JobListMenu<>( jobList )
, mRemoveDependencyAction( 0 )
, mShowHistoryAction( 0 )
, mSubmitVideoMakerAction( 0 )
, mModifyFrameRangeAction( 0 )
{
	setObjectName( "FreezerJobMenu" );
}

void FreezerJobMenu::slotAboutToShow()
{
	clear();

	JobList jobs = mJobList->mJobTree->selection();

	addAction( mJobList->RefreshAction );

	QMenu * filtersMenu = addMenu( "View Filters" );

	filtersMenu->addAction( mJobList->ShowMineAction );
	filtersMenu->addMenu( mJobList->mProjectFilterMenu );
	filtersMenu->addMenu( mJobList->mStatusFilterMenu);
	filtersMenu->addMenu( mJobList->mJobTypeFilterMenu );
	filtersMenu->addMenu( mJobList->mJobServiceFilterMenu );
	filtersMenu->addSeparator();

//	if( mFreezer->mAdminEnabled ) {
	mSetJobKeyListAction = filtersMenu->addAction( "Set Job Key List");
	mClearJobKeyListAction = filtersMenu->addAction( "Clear Job Key List");
//	}
	filtersMenu->addAction("Set Limit...", mJobList, SLOT(setLimit()) );

	FreezerMenuFactory::instance()->aboutToShow( this, true, true );
	addSeparator();
    QMenu * openWithMenu = addMenu("Open with..");

	if( JobViewerFactory::mJobViewerPlugins.size() ) {

		// replace with plugins driven system
		foreach( JobViewerPlugin * jvp, JobViewerFactory::mJobViewerPlugins.values() ) {
			QAction * action = new QAction( jvp->name(), this );
			action->setIcon( QIcon(jvp->icon()) );
			openWithMenu->addAction( action );
			mJobViewerActions[action] = jvp;
		}
		addSeparator();
	}
	
	if( jobs.size() == 1 )
		addAction( mJobList->ExploreJobFile );
	addAction( mJobList->ShowOutputAction );
	addAction( mJobList->FrameCyclerAction );
	addAction( mJobList->PdPlayerAction );

	if( jobs.size() == 1 ) {
		Job j = jobs[0];
		
		#ifdef Q_OS_WIN
		QString submitPath = "c:/blur/absubmit/fusionvideomakersubmit/submit.py";
		#else
		QString submitPath = "/mnt/storage/blur/cpp/apps/absubmit/fusionvideomakersubmit/submit.py";
		#endif
		if( !j.outputPath().isEmpty() && QFileInfo(j.outputPath()).suffix().contains( QRegExp( "^(jpg|jpeg|tif|tiff|exr|tga|targa|png)$", Qt::CaseInsensitive ) ) ) {
			mSubmitVideoMakerAction = addAction( "Submit Video Maker Job From Output..." );
			if( !QFile::exists(submitPath) )
				mSubmitVideoMakerAction->setEnabled(false);
		}
	}

	bool jobOwner = mJobList->currentJob().user() == User::currentUser();
	if( User::hasPerms( "Job", false ) || jobOwner )
		addAction("Job Info...", mJobList, SLOT(showJobInfo()) );

	addAction("Job Statistics...", mJobList, SLOT(showJobStatistics()) );

	if( jobs.size() == 1 )
		mShowHistoryAction = addAction("Job History...");

	addSeparator();

	// wrangler actions
//	if( jobs.size() == 1 && (User::hasPerms( "Job", true ) || jobOwner) )
		mModifyFrameRangeAction = addAction( "Modify frame range..." );

	addAction("Set Job(s) Priority...", mJobList, SLOT(setJobPriority()) );
	addAction( mJobList->ClearErrorsAction );
	if( !User::currentUser().userGroups().groups().contains( Group::recordByName("RenderOps") ) ) {
		mJobList->ClearErrorsAction->setEnabled( false );
	}

	bool allDeps = !jobs.isEmpty();

	QItemSelection itemSelection = mJobList->mJobTree->selectionModel()->selection();
	foreach( QItemSelectionRange sr, itemSelection ) {
		QModelIndex par = sr.parent();
		if( !par.isValid() || !JobTranslator::isType(par) ) {
			allDeps = false;
			break;
		}
	}

	if( allDeps ) {
		mRemoveDependencyAction = addAction( "Remove Dependency" );
		addSeparator();
	} else
		mRemoveDependencyAction = 0;

	addSeparator();

    // job control
	addAction( mJobList->ResumeAction );
	addAction( mJobList->PauseAction );
	addAction( mJobList->KillAction );
	addSeparator();
	addAction( mJobList->RestartAction );
	addSeparator();

	// low frequency use, put at bottom of menu
	if( jobs.size() == 1 && JobBatch(jobs[0]).isRecord() )
		addAction( "Save As Canned Batch Job...", mJobList, SLOT(saveCannedBatchJob()) );
}

void FreezerJobMenu::slotActionTriggered( QAction * action )
{
    if( !action ) return;
	if( action == mRemoveDependencyAction ) {
		QItemSelection itemSelection = mJobList->mJobTree->selectionModel()->selection();
		JobDepList toRemove;
		JobList checkReady;
		QModelIndexList indexesToRemove;
		foreach( QItemSelectionRange sr, itemSelection ) {
			Job job = mJobList->mJobTree->model()->getRecord( sr.parent() );
			checkReady += job;
			if( job.isRecord() ) {
				QModelIndex i = sr.topLeft();
				do {
					Job dep = mJobList->mJobTree->model()->getRecord(i);
					toRemove += JobDep::recordByJobAndDep(job,dep);
					LOG_5( "Job " + job.name() + " remove dependency on " + dep.name() );
					job.addHistory( QString("Remove dependency on %1 key %2").arg(dep.name()).arg(dep.key()) );

					i = i.sibling( i.row() + 1, 0 );
				} while( sr.contains(i) );
			}
		}
		// Remove the dependency links
		toRemove.remove();
		
		// Update any jobs that should now be ready
		foreach( Job j, checkReady ) {
			if( j.status() == "holding" ) {
				QString status = "ready";
				JobDepList deps = JobDep::recordsByJob(j);
				foreach( JobDep jobDep, deps ) {
					Job dep = jobDep.dep();
					if( dep.status() != "done" && dep.status() != "deleted" ) {
						status = "holding";
						break;
					}
				}
				if( status == "ready" ) {
					j.setStatus( status );
					j.commit();
				}
			}
		}
	} else if ( action == mShowHistoryAction ) {
		HostHistoryWindow * hhw = new HostHistoryWindow();
		hhw->setWindowTitle( "Job Execution History" );
		hhw->view()->setJobFilter( mJobList->mJobTree->selection()[0] );
		hhw->show();
	} else if( action == mSubmitVideoMakerAction ) {
		JobList jobs = mJobList->mJobTree->selection();
		if( jobs.size() == 1 ) {
			Job j = jobs[0];
			QStringList args;
#ifdef Q_OS_WIN
			QString workingPath = "c:/blur/absubmit/fusionvideomakersubmit/";
			QString executable = "python.exe";
#else
			// TODO: Need default install location for unix
			QString workingPath = "/mnt/storage/blur/cpp/apps/absubmit/fusionvideomakersubmit/";
			QString executable = "python";
#endif
			args << (workingPath + "submit.py");
			args << "sequencePath" << j.outputPath();
			args << "frameList" << (QString("%1-%2").arg(j.getValue("frameStart").toInt()).arg(j.getValue("frameEnd").toInt()));
			QProcess::startDetached( executable, args, workingPath );
		}
	} else if( action == mSetJobKeyListAction ) {
		bool okay = false;
		QString jobKeyList = QInputDialog::getText(mJobList,"Set Job List by job keys","Set Job List by job keys, comma or space separated", QLineEdit::Normal, mJobList->jobList().keyString(), &okay );
		if( okay )
			mJobList->setJobList(Job::table()->records(jobKeyList.remove(QRegExp("\\s+"))));
	} else if( action == mClearJobKeyListAction ) {
		mJobList->clearJobList();
	} else if( mJobViewerActions.contains(action) ) {
		mJobViewerActions[action]->view(mJobList->mJobTree->selection());
	} else if( action == mModifyFrameRangeAction ) {
		foreach( Job job, mJobList->mJobTree->selection() )
			modifyFrameRange( job );
	}
}

void FreezerJobMenu::modifyFrameRange( Job j )
{
	JobTaskList tasks = j.jobTasks();
	if( j.packetType() == "preassigned" ) {
		HostSelector * hs = new HostSelector( mJobList );
		hs->setHostList( tasks.filter( "status", "cancelled", false ).hosts() );
		if( hs->exec() == QDialog::Accepted ) {
			j.changePreassignedTaskListWithStatusPrompt( hs->hostList(), mJobList );
		}
		delete hs;
	} else {
		QMap<JobOutput,JobTaskList> tasksByOutput = tasks.groupedBy<JobOutput,JobTaskList,uint,JobOutput>("fkeyjoboutput");
		if( tasksByOutput.size() != 1 ) {
			QMessageBox::critical( mJobList, "Operation not supported", "Freezer is currently unable to modify frame ranges for job that have multiple outputs." );
		} else {
			QString frameRange = compactNumberList(tasks.filter( "status", "cancelled", false ).frameNumbers());
			do {
				bool valid = false;
				frameRange = QInputDialog::getText( mJobList, "Enter modified frame range", "Enter modified frame range", QLineEdit::Normal, frameRange, &valid );
				if( !valid ) break;
				QList<int> newFrameNumbers = expandNumberList( frameRange, &valid );
				if( valid ) {
					j.changeFrameRange( newFrameNumbers, tasks[0].jobOutput() );
					if( j.status() == "done" )
						Job::updateJobStatuses( j, "started", false, false );
					break;
				} else
					QMessageBox::warning(mJobList, "Invalid frame range", "The frame range entered was invalid: " + frameRange);
			} while(true);
		}
	}
}

CannedBatchJobMenu::CannedBatchJobMenu(HostListWidget * hostList)
: HostListMenu<>( hostList, "Canned Batch Jobs" )
{}

void CannedBatchJobMenu::slotAboutToShow()
{
	clear();
	QMap<QString,RecordList> groupedCans = JobCannedBatch::select().groupedBy( "group" );
	QMap<QString, RecordList>::const_iterator i = groupedCans.constBegin();
	for (;i != groupedCans.constEnd(); ++i) {
		QMenu * group = addMenu( i.key() );
		RecordList rl = i.value();
		foreach( JobCannedBatch jcb, rl ) {
			QAction * a = group->addAction( jcb.name() );
			a->setProperty( "record", qVariantFromValue<Record>(jcb) );
		}
	}
	FreezerMenuFactory::instance()->aboutToShow( this, true, true );
}

void CannedBatchJobMenu::slotActionTriggered( QAction * action )
{
	JobCannedBatch jcb = qvariant_cast<Record>( action->property( "record" ) );
	BatchSubmitDialog * bsd = new BatchSubmitDialog( this );
	bsd->setName( jcb.name() );
	bsd->setCommand( jcb.cmd() );
	bsd->setCannedBatchGroup( jcb.group() );
	bsd->setHostList( mHostList->hostTreeView()->selection() );
	bsd->exec();
	delete bsd;
}

HostPluginMenu::HostPluginMenu(HostListWidget * hostList)
: HostListMenu<>(hostList, "Host Plugins")
{}

void HostPluginMenu::slotAboutToShow()
{
	clear();

	foreach (HostViewerPlugin * hvp, HostViewerFactory::mHostViewerPlugins.values() ) {
		QAction * action = new QAction( hvp->name(), this );
		action->setIcon( QIcon(hvp->icon()) );
		addAction( action );
		mHostPluginActions[action] = hvp;
	}
}

void HostPluginMenu::slotActionTriggered( QAction * action )
{
	if (!action) return;

	if (mHostPluginActions.contains(action))
		mHostPluginActions[action]->view( HostList(mHostList->hostTreeView()->selection()) );
}

TailServiceLogMenu::TailServiceLogMenu(HostListWidget * hostList)
: HostListMenu<>( hostList, "Tail Service Log..." )
{
}

void TailServiceLogMenu::slotAboutToShow()
{
	clear();
	FreezerMenuFactory::instance()->aboutToShow( this, true, true );
	HostList hl = mHostList->hostTreeView()->selection();
	if( hl.size() == 1 ) {
		Host h = hl[0];
		// Only list services with a non-null remoteLogPort
		foreach( HostService hs, h.hostServices().filter( "remoteLogPort", 0, false ) ) {
			QAction * tailAction = addAction( hs.service().service() );
			tailAction->setProperty( "record", qVariantFromValue<Record>(hs) );
		}
	}
}

void TailServiceLogMenu::slotActionTriggered( QAction * action )
{
	HostService hs = qvariant_cast<Record>( action->property( "record" ) );
	if( hs.isRecord() ) {
		RemoteTailWindow * tailWindow = new RemoteTailWindow();
		tailWindow->setServiceName( hs.service().service() );
		RemoteTailWidget * tailWidget = tailWindow->tailWidget();
		tailWidget->connectToHost( hs.host().name(), hs.remoteLogPort() );
		tailWidget->setFileName( "APPLICATION_LOG" );
		tailWindow->show();
	}
}

FreezerHostMenu::FreezerHostMenu(HostListWidget * hostList)
: HostListMenu<>( hostList )
, mShowHistoryAction( 0 )
{}

void FreezerHostMenu::slotAboutToShow()
{
	clear();
	addAction( mHostList->RefreshHostsAction );

	HostList hl = mHostList->hostTreeView()->selection();

    // Move plugins to a separate sub menu
    addMenu( mHostList->mHostPluginMenu );
	addSeparator();
	addAction( mHostList->HostOnlineAction );
	addAction( mHostList->HostOfflineAction );
	addAction( mHostList->HostOfflineWhenDoneAction );
	addAction( mHostList->HostRestartAction );
	addAction( mHostList->HostRestartWhenDoneAction );
	addAction( mHostList->HostRebootAction );
	addAction( mHostList->HostRebootWhenDoneAction );
	addAction( mHostList->HostShutdownAction );
	addAction( mHostList->HostShutdownWhenDoneAction );
	addAction( mHostList->HostMaintenanceEnableAction );
	addSeparator();

	addAction( mHostList->ShowJobsAction );
	if( hl.size() == 1 )
			addAction( mHostList->ShowHostErrorsAction );
	addAction( mHostList->ClearHostErrorsAction );
	addAction( mHostList->ClearHostErrorsSetOfflineAction );
//	addAction( mHostList->ClientUpdateAction );
	
	addSeparator();
	addAction( mHostList->VNCHostsAction );
	addAction( mHostList->ShowJobsAction );
	if( hl.size() == 1 )
		addAction( mHostList->ShowHostErrorsAction );
	if( User::hasPerms( "Host", false ) )
		addAction( mHostList->ShowHostInfoAction );
	mShowHistoryAction = addAction( "Show History..." );
	addSeparator();
	addAction( mHostList->SubmitBatchJobAction );
	addMenu( mHostList->mCannedBatchJobMenu );
	mCancelBatchJobTasksAction = addAction( "Cancel Hosts' Batch Job Tasks..." );

	if( true ) { //User::hasPerms( "Host", true ) ) {
	addSeparator();
		mNewHostAction = addAction( "New Host" );
		if( hl.size() == 1 )
			mEditHostsAction  = addAction( "Edit Host" );
		if( hl.size() )
			mRemoveHostsAction = addAction( "Delete Host" );
	}

	addMenu( mHostList->mHostServiceFilterMenu );
	FreezerMenuFactory::instance()->aboutToShow( this, true, true );
}

void FreezerHostMenu::slotActionTriggered( QAction * action )
{
	if( !action ) return;
	if( action == mHostList->SubmitBatchJobAction ) {
		BatchSubmitDialog * bsd = new BatchSubmitDialog( mHostList );
		bsd->setHostList( mHostList->hostTreeView()->selection() );
		bsd->exec();
		delete bsd;
	} else if( action == mHostList->ShowHostInfoAction ) {
		mHostList->restorePopup( RecordPropValTree::showRecords( mHostList->hostTreeView()->selection(), mHostList, User::hasPerms( "Host", true ) ) );
	} else if( action == mHostList->ClearHostErrorsSetOfflineAction ) {
		clearHostErrorsAndSetOffline( mHostList->hostTreeView()->selection(), true );
	} else if( action == mHostList->ClearHostErrorsAction ) {
		clearHostErrorsAndSetOffline( mHostList->hostTreeView()->selection(), false );
	} else if( action == mHostList->ShowHostErrorsAction ) {
		HostList hl = mHostList->hostTreeView()->selection();
		HostErrorWindow * hew = new HostErrorWindow( mHostList );
		hew->setHost( hl[0] );
		hew->show();
	} else if( action == mShowHistoryAction ) {
		HostHistoryWindow * hhw = new HostHistoryWindow();
		hhw->setWindowTitle( "Host Activity History" );
		hhw->view()->setHostFilter( mHostList->hostTreeView()->selection() );
		hhw->show();
	} else if( mHostViewerActions.contains(action) ) {
		mHostViewerActions[action]->view( mHostList->hostTreeView()->selection() );
	} else if( action == mNewHostAction ) {
		Database::current()->beginTransaction( "Create Host" );
		HostDialog * hd = new HostDialog( this );
		if( hd->exec() == QDialog::Accepted )
			Database::current()->commitTransaction();
		else
			Database::current()->rollbackTransaction();
		delete hd;
	} else if( action == mEditHostsAction ) {
		Database::current()->beginTransaction( "Modified Host Info" );
		HostDialog * hd = new HostDialog( this );
		HostList hl = mHostList->hostTreeView()->selection();
		hd->setHost( hl[0] );
		if( hd->exec() == QDialog::Accepted )
			Database::current()->commitTransaction();
		else
			Database::current()->rollbackTransaction();
		delete hd;
	} else if( action == mRemoveHostsAction ) {
		Database::current()->beginTransaction( "Deleted Host" );
		HostList hl = mHostList->hostTreeView()->selection();
		hl.remove();
		Database::current()->commitTransaction();
	} else if( action == mCancelBatchJobTasksAction ) {
		JobTaskList toCommit;
		Expression e = JobTask::c.Host.in(mHostList->hostTreeView()->selection())
			& JobTask::c.Job.in(
				Query( Job::c.Key, JobBatch::table(), Job::c.Status.in(QStringList() << "new" << "ready" << "started" << "suspended") ) );
		foreach( JobTask jt, e.select() ) {
			if( jt.status() == "new" || jt.status() == "suspended" )
				toCommit += jt.setStatus( "cancelled" );
		}
		toCommit.commit();
	}
}

static void createHostViewWithSelection(QWidget * widget, HostList hosts )
{
	MainWindow * mw = qobject_cast<MainWindow*>(widget->window());
	if( mw ) {
		HostListWidget * hostListView = new HostListWidget(mw);
		hostListView->selectHosts( hosts );
		mw->insertView( hostListView );
		mw->setCurrentView( hostListView );
	}
}

FreezerTaskMenu::FreezerTaskMenu(JobListWidget * jobList)
: JobListMenu<>(jobList)
, mInfoAction( 0 )
, mRerenderFramesAction( 0 )
, mSuspendFramesAction( 0 )
, mCancelFramesAction( 0 )
, mShowLogAction( 0 )
, mCopyCommandAction( 0 )
, mSelectHostsAction( 0 )
, mVncHostsAction( 0 )
, mShowHistoryAction( 0 )
{
}

void FreezerTaskMenu::slotAboutToShow()
{
	clear();
	mTasks = mJobList->mFrameTree->selection();

	if( JobViewerFactory::mJobViewerPlugins.size() ) {
		QMenu * openWithMenu = addMenu("Open output with..");
		foreach( JobViewerPlugin * jvp, JobViewerFactory::mJobViewerPlugins.values() ) {
			QAction * action = new QAction( jvp->name(), this );
			action->setIcon( QIcon(jvp->icon()) );
			openWithMenu->addAction( action );
			mJobViewerActions[action] = jvp;
		}
	}
	
	if( HostViewerFactory::mHostViewerPlugins.size() ) {
		QMenu * connectWithMenu = addMenu("Connect to host..");
		foreach( HostViewerPlugin * hvp, HostViewerFactory::mHostViewerPlugins.values() ) {
			QAction * action = new QAction( hvp->name(), this );
			action->setIcon( QIcon(hvp->icon()) );
			connectWithMenu->addAction( action );
			mHostViewerActions[action] = hvp;
		}
	}
	
	addSeparator();

	if( /*( User::hasPerms( "JobTask", false ) || mJobList->currentJob().user() == User::currentUser() ) &&*/
		 mTasks.size() == 1 ) {
		mInfoAction = addAction( "Task Info..." );

		mShowLogAction = addAction( "Show Log..." );

		QMenu * logMenu = addMenu("Show Log With...");
		foreach( FrameViewerPlugin * fvp, FrameViewerFactory::mFrameViewerPlugins.values() ) {
			QAction * action = new QAction( fvp->name(), this );
			action->setIcon( QIcon(fvp->icon()) );
			logMenu->addAction( action );
			mFrameViewerActions[action] = fvp;
		}
		mShowHistoryAction = addAction( "Show History..." );
		mCopyCommandAction = addAction( "Copy command" );
	}
		
	addAction( mJobList->ShowOutputAction );
	addAction( mJobList->FrameCyclerAction );
	addAction( mJobList->PdPlayerAction );

	mVncHostsAction = addAction( "Vnc Hosts" );
	mVncHostsAction->setIcon( QIcon( ":/images/vnc_hosts.png" ) );

	if( mTasks.size() )
		mSelectHostsAction = addAction( "Select Host(s)" );

	addSeparator();

	bool enabled = !mTasks.isEmpty();

		//&& (User::currentUser() ==  mJobList->currentJob().user() || User::hasPerms( "JobTask", true ) ) )
	if( (QStringList() << "ready" << "started" << "done" << "suspended").contains(mJobList->currentJob().status()) )
	{
		mRerenderFramesAction = addAction( "Rerender Frame" + QString(mTasks.size()>1 ? "s" : "") );
		mRerenderFramesAction->setEnabled( enabled );
		mSuspendFramesAction = addAction( "Suspend Selected Frames" );
		mSuspendFramesAction->setEnabled( enabled );
		mCancelFramesAction = addAction( "Cancel Selected Frames" );
		mCancelFramesAction->setEnabled( enabled );
	}

	addSeparator();
	QMenu * multiTaskMenu = addMenu("Run on frames...");
	foreach( MultiFrameViewerPlugin * mfvp, MultiFrameViewerFactory::mMultiFrameViewerPlugins.values() ) {
		QAction * action = new QAction( mfvp->name(), this );
		action->setIcon( QIcon(mfvp->icon()) );
		multiTaskMenu->addAction( action );
		mMultiFrameViewerActions[action] = mfvp;
	}
	
	mJobList->FrameCyclerAction->setEnabled( enabled );
	mVncHostsAction->setEnabled( enabled );

	FreezerMenuFactory::instance()->aboutToShow( this, true, true );
}

static QStringList taskListToFrameList( JobTaskList taskList )
{
	QStringList frameList;
	foreach( JobTask jt, taskList )
		frameList.append(QString::number(jt.frameNumber()));
	return frameList;
}

void FreezerTaskMenu::slotActionTriggered( QAction * action )
{
    if( !action ) return;
	if( action==mRerenderFramesAction ){

		// Filter out the statuses that are already new.  Otherwise there is a
		// race condition where the manager marks them assigned then we clear
		// the fkeyhost.  This wouldn't be a problem if we included columns in the
		// update that haven't changed locally.
		mTasks = mTasks.filter( "status", "new", false );

		QMap<Job, JobTaskList> tasksByJob = mTasks.groupedByForeignKey<Job,JobTaskList>( "fkeyjob" );
		foreach( Job j, tasksByJob.keys() ) {
			
			JobTaskList tasks = tasksByJob[j];
			
			QStringList frameList;
			foreach( JobTask jt, tasks )
				frameList += QString::number( jt.frameNumber() );

			if( QMessageBox::warning( this, "Rerender Frames", "Are you sure that you want to re-render the following frames?\nJob:" + j.name() + "\nFrames: " + frameList.join(","),
				QMessageBox::Yes, QMessageBox::Cancel ) != QMessageBox::Yes )
				return;
	
			JobTaskAssignmentList assignments = tasks.jobTaskAssignments();
			assignments.setJobAssignmentStatuses( JobAssignmentStatus::recordByName( "cancelled" ) );
			assignments.commit();
			tasks.setStatuses( "new" );
			tasks.setHosts( Host() );
			tasks.setJobTaskAssignments( JobTaskAssignment() );
			tasks.commit();
	
			if( j.status() == "done" ) {
				j.setStatus( "started" );
				j.commit();
			}
		
			j.addHistory( "Rerender Frames: " + taskListToFrameList(tasks).join(",") );
		}

		mJobList->refreshFrameList(false);
		mJobList->setStatusBarMessage( "Frames marked as 'new' for re-render" );
	}
	else if( action == mInfoAction )
		mJobList->restorePopup( RecordPropValTree::showRecords( mTasks, mJobList, User::hasPerms( "JobTask", true ) || mJobList->currentJob().user() == User::currentUser() ) );
	else if( action == mCancelFramesAction ) {
		QMap<Job, JobTaskList> tasksByJob = mTasks.groupedByForeignKey<Job,JobTaskList>( "fkeyjob" );
		foreach( Job j, tasksByJob.keys() ) {
			JobTaskList tasks = tasksByJob[j];
			QStringList frameList;
			foreach( JobTask jt, tasks )
				frameList += QString::number( jt.frameNumber() );
			if( QMessageBox::warning( this, "Cancel Frames", "Are you sure that you want to cancel the following frames?\n"
				"The Frames will be marked 'cancelled', and will not be rendered.  You can select rerender later, but the job will be marked as done without the canceled frames being rendered, and if the delete on complete option is set you will not have a chance to rerender the frames.\nJob: " + j.name() + "\nFrames: " + frameList.join(","),
				QMessageBox::Yes, QMessageBox::Cancel ) != QMessageBox::Yes )
				return;
	
        // Reload the tasks just in case their assignment status has changed between the dialog popping up and the user pressing "yes"
			tasks.reload();
			foreach( JobAssignment ass, tasks.jobTaskAssignments().jobAssignments() )
				Database::current()->exec("SELECT cancel_job_assignment("+QString::number(ass.key())+",'cancelled','cancelled')");
			tasks.setStatuses("cancelled");
			tasks.commit();
			j.addHistory(  "Cancel Frames: " + taskListToFrameList(tasks).join(",") );
		}

		//mJobList->refreshCurrentTab();
		mJobList->setStatusBarMessage( "Frames marked as 'cancelled'" );
	}
	else if( action == mSuspendFramesAction ) {
		// Reload the tasks in case the assignment status has changed.
		mTasks.reload();
		QMap<Job, JobTaskList> tasksByJob = mTasks.groupedByForeignKey<Job,JobTaskList>( "fkeyjob" );
		foreach( Job j, tasksByJob.keys() ) {
			JobTaskList tasks = tasksByJob[j];
			foreach( JobAssignment ass, tasks.jobTaskAssignments().jobAssignments() )
				Database::current()->exec("SELECT cancel_job_assignment("+QString::number(ass.key())+",'cancelled','suspended')");
			tasks.setStatuses("suspended");
			tasks.commit();
			j.addHistory(  "Suspend Frames: " + taskListToFrameList(tasks).join(",") );
		}
		mJobList->setStatusBarMessage( QString::number( mTasks.size() ) + " frame" + QString(mTasks.size() > 1 ? "s" : "") + " suspended" );
	}
	else if( action == mShowLogAction && mTasks.size() == 1 ) {
		JobTask jt = mTasks[0];
		JobAssignmentWindow * jaw = new JobAssignmentWindow();
		JobAssignment ja = jt.jobTaskAssignment().jobAssignment();

		jaw->jaWidget()->setJobAssignment( ja );
		jaw->setWindowTitle("Job Log for frame "+QString::number(jt.frameNumber()));
		jaw->show();
	}
	else if( action == mCopyCommandAction && mTasks.size() == 1 ) {
		JobTask jt = mTasks[0];
		QClipboard * cb = QApplication::clipboard();
		cb->setText( jt.jobTaskAssignment().jobAssignment().command() );
	}
	else if( action == mSelectHostsAction )
		createHostViewWithSelection( mJobList, mTasks.hosts() );
	else if( action == mVncHostsAction ) {
		foreach( Host h, JobTaskList(mJobList->mFrameTree->selection()).hosts() )
			vncHost( h.name() );
	}
	else if( action == mShowHistoryAction ) {
		HostHistoryWindow * hhw = new HostHistoryWindow();
		hhw->setWindowTitle( "Task Execution History" );
		hhw->view()->setTaskFilter( mJobList->mFrameTree->selection()[0] );
		hhw->show();
	} else if( mJobViewerActions.contains(action) ) {
		mJobViewerActions[action]->view(mJobList->mJobTree->selection());
	} else if( mHostViewerActions.contains(action) ) {
		mHostViewerActions[action]->view( JobTaskList(mJobList->mFrameTree->selection()).hosts() );
	} else if( mFrameViewerActions.contains(action) ) {
		JobTaskList jtl = mJobList->mFrameTree->selection();
			mFrameViewerActions[action]->view( jtl[0].jobTaskAssignment().jobAssignment() );
	} else if( mMultiFrameViewerActions.contains(action) ) {
		JobTaskList jtl = mJobList->mFrameTree->selection();
		mMultiFrameViewerActions[action]->view( mTasks );
	}
}

FreezerErrorMenu::FreezerErrorMenu(QWidget * parent, JobErrorList selection, JobErrorList all)
: FreezerMenu(parent)
, mSelection( selection )
, mAll( all )
, mClearSelected( 0 )
, mClearAll( 0 )
, mCopyText( 0 )
, mShowLog( 0 )
, mSelectHosts( 0 )
, mRemoveHosts( 0 )
, mClearHostErrorsAndOffline( 0 )
, mClearHostErrors( 0 )
, mShowErrorInfo( 0 )
, mSetJobKeyListAction( 0 )
, mClearJobKeyListAction( 0 )
, mServiceFilterMenu( 0 )
, mJobTypeFilterMenu( 0 )
{
}

void FreezerErrorMenu::setErrors( JobErrorList selection, JobErrorList allErrors )
{
	mSelection = selection;
	mAll = allErrors;
}

void FreezerErrorMenu::slotAboutToShow()
{
	clear();
	
	bool isJobListChild = parent()->inherits( "JobListWidget" );
	bool isErrorViewChild = parent()->inherits( "ErrorListWidget" );
	bool hasSelection = mSelection.size(), hasErrors = mAll.size();

	mShowLog = addAction("Show Log...");
	mShowLog->setEnabled( mSelection.size() == 1 );

	QMenu * logMenu = addMenu("Show Log With...");
	foreach( FrameViewerPlugin * fvp, FrameViewerFactory::mFrameViewerPlugins.values() ) {
		QAction * action = new QAction( fvp->name(), this );
		action->setIcon( QIcon(fvp->icon()) );
		logMenu->addAction( action );
		mFrameViewerActions[action] = fvp;
	}
	
	if( isErrorViewChild ) {
		// TODO: Should be available for the job list errors tab also
		addAction( QIcon( ":/images/refresh" ), "Refresh", parent(), SLOT( refresh() ) );
		
		ErrorListWidget * errorList = qobject_cast<ErrorListWidget*>(parent());
		QMenu * filtersMenu = addMenu( "View Filters" );
		if( !mServiceFilterMenu )
			mServiceFilterMenu = new ErrorServiceFilterMenu( errorList );
		filtersMenu->addMenu(mServiceFilterMenu);
		if( !mJobTypeFilterMenu )
			mJobTypeFilterMenu = new ErrorListJobTypeFilterMenu( errorList );
		filtersMenu->addMenu(mJobTypeFilterMenu);
		filtersMenu->addSeparator();
		mSetJobKeyListAction = filtersMenu->addAction( "Set Job Key List");
		mClearJobKeyListAction = filtersMenu->addAction( "Clear Job Key List");
		mSetLimitAction = filtersMenu->addAction("Set Limit...");
	}

	if( isJobListChild ) {
		JobListWidget * jlw = qobject_cast<JobListWidget*>(parent());
		if( jlw )
			addAction(jlw->ShowClearedErrorsAction);
		mVncHostsAction = addAction( "Vnc Hosts" );
		mVncHostsAction->setIcon( QIcon( ":/images/vnc_hosts.png" ) );
		mVncHostsAction->setEnabled( hasSelection );
	}
	
	mClearSelected = addAction("Clear Selected Errors");
	mClearSelected->setEnabled( hasSelection );

	if( isJobListChild ) {
		mClearAll = addAction( "Clear All Errors" );
		mClearAll->setEnabled( hasErrors );
		addSeparator();
	}

	if( !User::currentUser().userGroups().groups().contains( Group::recordByName("RenderOps") ) ) {
		mClearSelected->setEnabled( false );
		if( mClearAll )
			mClearAll->setEnabled( false );
	}

	mCopyText = addAction("Copy Text of Selected Errors");
	mCopyText->setEnabled( hasSelection );
	
	mShowErrorInfo = addAction("Error Info...");
	mShowErrorInfo->setEnabled( mSelection.size() == 1 );

	if( isJobListChild || isErrorViewChild ) {
		mSelectHosts = addAction("Select Host(s)");
		mSelectHosts->setEnabled( hasSelection );
	}

	addSeparator();

	if( isJobListChild ) {
		mRemoveHosts = addAction( "Exclude Selected Hosts From Selected Job(s)" );
		mRemoveHosts->setEnabled( hasSelection );
	}

	if( isJobListChild ) {
		mClearHostErrors = addAction( "Clear All Errors From Host");
		mClearHostErrors->setEnabled( mSelection.size() == 1 );
		mClearHostErrorsAndOffline = addAction( "Clear All Errors From Host and Set It Offline" );
		mClearHostErrorsAndOffline->setEnabled( mSelection.size() == 1 );
	}

	FreezerMenuFactory::instance()->aboutToShow( this, true, true );
}

void FreezerErrorMenu::slotActionTriggered( QAction * action )
{
	if( !action ) return;

	if( (action==mClearAll) || ( action == mClearSelected ) ){
		if( action == mClearAll ) {
			if( QMessageBox::warning( this, "Clear All Errors", "Are you sure that you want to clear all errors for selected jobs",
				QMessageBox::Yes, QMessageBox::Cancel ) != QMessageBox::Yes )
				return;
			mAll.setCleared( true ).commit();
			foreach( Job j, mAll.jobs() )
				j.addHistory( "All errors cleared" );
		} else {
			mSelection.setCleared( true ).commit();
			foreach( Job j, mSelection.jobs() )
				j.addHistory( "Selected errors cleared" );
		}
		JobListWidget * jlw = qobject_cast<JobListWidget*>(parent());
		if( jlw )
			jlw->refreshCurrentTab();
	}
	else if( action == mCopyText ) {
		QStringList errorList;
		foreach( JobError je, mSelection ) {
			QStringList parts;
			for(int n=0; n<4;n++ ) {
				switch( n ) {
					case 0: parts += je.host().name(); break;
					case 1: parts += je.lastOccurrence().toString(); break;
					case 2: parts += je.frames(); break;
					case 3: parts += je.message().replace( "\r", "" ).trimmed(); break;
				}
			}
			errorList += "[" + parts[0] + "; " + parts[1] + "; Frame " + parts[2] + "]\n" + parts[3];
		}
		QApplication::clipboard()->setText( errorList.join("\n"), QClipboard::Clipboard );
	} else if( action == mRemoveHosts ) {
		JobList jobs = mSelection.jobs().unique();
		foreach( Job job, jobs ) {
			HostList exclude = JobErrorList(mSelection).hosts();
			HostList hl = hostListFromString(job.hostList());
	
			// If the host list is empty, set to all the host that can perform this jobtype
			if( hl.isEmpty() ) hl = job.jobType().service().hostServices().hosts();
	
			hl -= exclude;
	
			job.setHostList( hl.names().join(",") );
			job.commit();
			job.addHistory("Host(s) excluded from job");
		}
	} else if( action == mClearHostErrorsAndOffline ) {
		clearHostErrorsAndSetOffline( mSelection.hosts(), true);
	} else if( action == mClearHostErrors ) {
		clearHostErrorsAndSetOffline( mSelection.hosts(), false);
	} else if ( mSelection.size() > 0 && action == mShowLog ) {
		JobAssignmentWindow * jaw = new JobAssignmentWindow();
		jaw->jaWidget()->setJobAssignment( mSelection[0].jobAssignment() );
		jaw->setWindowTitle("Job Log for errored task(s) "+ mSelection[0].frames());
		jaw->show();
	} else if ( action == mSelectHosts ) {
		FreezerView * afw = qobject_cast<FreezerView*>(parent());
		if( afw )
			createHostViewWithSelection( afw, mSelection.hosts() );
	} else if( action == mVncHostsAction ) {
		foreach( Host h, mSelection.hosts() )
			vncHost( h.name() );
	} else if( action == mShowErrorInfo ) {
		FreezerView * afw = qobject_cast<FreezerView*>(parent());
		if( afw )
			afw->restorePopup( RecordPropValTree::showRecords( mSelection[0], this, User::hasPerms( "JobError", true ) ) );
	} else if( mSelection.size() > 0 && mFrameViewerActions.contains(action) ) {
		JobError je(mSelection[0]);
		mFrameViewerActions[action]->view( je.jobAssignment() );
	} else if( action == mSetJobKeyListAction ) {
		bool okay = false;
		QString jobKeyList = QInputDialog::getText(qobject_cast<QWidget*>(parent()),"Set Job List by job keys","Set Job List by job keys, comma or space separated", QLineEdit::Normal, qobject_cast<ErrorListWidget*>(parent())->jobFilter().keyString(), &okay );
		if( okay )
			qobject_cast<ErrorListWidget*>(parent())->setJobFilter(Job::table()->records(jobKeyList.remove(QRegExp("\\s+"))));
	} else if( action == mClearJobKeyListAction ) {
		qobject_cast<ErrorListWidget*>(parent())->setJobFilter(JobList());
	} else if( action == mSetLimitAction ) {
		ErrorListWidget * errorList = qobject_cast<ErrorListWidget*>(parent());
		if( errorList ) {
			bool ok;
			int limit = QInputDialog::getInteger( this, "Set Error Limit", "Enter Maximum number of errors to display", errorList->mLimit, 1, 100000, 1, &ok );
			if( ok ) {
				errorList->mLimit = limit;
				errorList->refresh();
			}
		}
	}
}

