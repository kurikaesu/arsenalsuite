
#include <qapplication.h>
#include <qaction.h>
#include <qfileinfo.h>
#include <qinputdialog.h>
#include <qmainwindow.h>
#include <qmessagebox.h>
#include <qprocess.h>
#include <qsplitter.h>
#include <qtimer.h>
#include <qtoolbar.h>
#include <qclipboard.h>
#include <qwebview.h>
#include <qwebframe.h>

#include <stdlib.h>

#include "blurqt.h"
#include "database.h"
#include "freezercore.h"
#include "path.h"
#include "process.h"

#include "group.h"
#include "jobbatch.h"
#include "jobcommandhistory.h"
#include "jobhistory.h"
#include "jobhistorytype.h"
#include "jobtype.h"
#include "projectstatus.h"
#include "service.h"
#include "user.h"
#include "usergroup.h"

#include "busywidget.h"
#include "filteredit.h"
#include "modelgrouper.h"
#include "recordtreeview.h"
#include "recordfilterwidget.h"
#include "recordpropvaltree.h"

#ifdef USE_GRAPHVIZ
#include "gvgraph.h"
#endif

#include "assfreezermenus.h"
#include "batchsubmitdialog.h"
#include "hostlistwidget.h"
#include "imageview.h"
#include "items.h"
#include "joblistwidget.h"
#include "jobstatwidget.h"
#include "mainwindow.h"
#include "tabtoolbar.h"
#include "threadtasks.h"

#include "usernotifydialog.h"
#include "joberrorswidgetfactory.h"
#include "joberrorswidgetplugin.h"

#include "jobframestabwidgetfactory.h"
#include "jobframestabwidgetplugin.h"

#ifdef LoadImage
#undef LoadImage
#endif

JobListWidget::SharedData * JobListWidget::mSharedData = 0;

JobListWidget::JobListWidget( QWidget * parent )
: FreezerView( parent )
, mJobFilterEdit( 0 )
, mToolBar( 0 )
, mViewsInitialized( false )
, mJobTaskRunning( false )
, mQueuedJobRefresh( false )
, mShowClearedErrors( false )
, mFrameTask( 0 )
, mJobTask( 0 )
, mPartialFrameTask( 0 )
, mStaticDataRetrieved( false )
, mJobMenu( 0 )
, mStatusFilterMenu( 0 )
, mProjectFilterMenu( 0 )
, mJobTypeFilterMenu( 0 )
, mTaskMenu( 0 )
, mErrorMenu( 0 )
{
	if( !mSharedData ) {
		mSharedData = new SharedData;
		mSharedData->mRefCount = 1;
	} else
		mSharedData->mRefCount++;

	setupUi(this);
}

JobListWidget::~JobListWidget()
{
	if( mSharedData->mRefCount-- == 0 )
		delete mSharedData;
}

QString JobListWidget::viewType() const
{
	return "JobList";
}

void JobListWidget::initializeViews(IniConfig & ini)
{
	if( !mViewsInitialized ) {
		mViewsInitialized = true;
		
		mTabToolBar = new TabToolBar( mJobTabWidget, mImageView );
		mTabToolBar->setMaximumHeight(22);

		// Add the tabToolBar to the Image Viewer tab
		mImageViewerLayout->addWidget(mTabToolBar);

		mGraphs = new QWebView(this);
		QUrl startURL = QUrl("resources/graph/index.html");
		mGraphs->load(startURL);
		mGraphLayout->addWidget(mGraphs);

		//mDepsView->load(QUrl("resources/deps/index.html"));

		RefreshAction = new QAction( "Refresh Job(s)", this );
		RefreshAction->setIcon( QIcon( ":/images/refresh" ) );

		KillAction = new QAction( "Remove Selected Jobs", this );
		KillAction->setIcon( QIcon( ":/images/kill" ) );
		PauseAction = new QAction( "Pause Selected Jobs", this );
		PauseAction->setIcon( QIcon( ":/images/pause" ) );
		ResumeAction = new QAction( "Resume Selected Jobs", this );
		ResumeAction->setIcon( QIcon( ":/images/resume" ) );
		RestartAction = new QAction( "Restart Job(s)", this );
		RestartAction->setIcon( QIcon( ":/images/restart" ) );
		ShowOutputAction = new QAction( "Show Output Folder", this );
		ShowOutputAction->setIcon( QIcon( ":/images/explorer" ) );
		ShowMineAction = new QAction( "View My Jobs", this );
		ShowMineAction->setCheckable( TRUE );
		ShowMineAction->setIcon( QIcon( ":/images/show_mine" ) );
		WhoAmIAction = new QAction( "Who Am I?", this );
		WhoAmIAction->setIcon( QIcon( ":/images/who_am_i" ) );
		FrameCyclerAction = new QAction( "Frame Cycler", this );
		FrameCyclerAction->setIcon( QIcon( ":/images/imagecycler.png" ) );
		PdPlayerAction = new QAction( "Pd Player", this );
		//PdPlayerAction->setIcon( QIcon( ":/images/imagecycler.png" ) );
		ClearErrorsAction = new QAction( "Clear Job Errors", this );
		ShowClearedErrorsAction = new QAction( "Show Cleared Errors", this );
		ShowClearedErrorsAction->setCheckable(true);
		ShowClearedErrorsAction->setChecked(false);
		connect( ShowClearedErrorsAction, SIGNAL( toggled( bool ) ), SLOT( refresh() ) );

		ExploreJobFile = new QAction( "Show Job File in Explorer", this );
		ExploreJobFile->setIcon( QIcon( ":/images/explorer" ) );
		DependencyTreeEnabledAction = new QAction( "Show Dependency Tree", this );
		DependencyTreeEnabledAction->setCheckable( true );
		DependencyTreeEnabledAction->setIcon( QIcon(":/images/dependencytree") );
		connect( DependencyTreeEnabledAction, SIGNAL( toggled( bool ) ), SLOT( setDependencyTreeEnabled( bool ) ) );

		FilterAction = new QAction( "&Filter", this );
		FilterAction->setCheckable( TRUE );
		FilterAction->setIcon( QIcon( ":images/filter" ) );
		FilterAction->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_F ) );
		connect( FilterAction, SIGNAL( triggered(bool) ), SLOT( toggleFilter(bool) ) );

		NewViewFromSelectionAction = new QAction( "New View From Selection", this );
		connect( NewViewFromSelectionAction, SIGNAL( triggered(bool) ), SLOT( createNewViewFromSelection() ) );

		FilterClearAction = new QAction( "Clear Filters", this );
		FilterClearAction->setShortcut( QKeySequence( Qt::CTRL + Qt::SHIFT + Qt::Key_F ) );
		FilterClearAction->setIcon( QIcon( ":images/filterclear" ) );
		connect( FilterClearAction, SIGNAL( triggered(bool) ), SLOT( clearFilters() ) );

		NewViewFromSelectionAction = new QAction( "New View From Selection", this );
		NewViewFromSelectionAction->setIcon( QIcon( ":/images/newview" ) );

		connect( NewViewFromSelectionAction, SIGNAL( triggered(bool) ), SLOT( createNewViewFromSelection() ) );
		mJobFilterEdit = new FilterEdit( this, Job::c.Name, "Job Name Filter:" );
		connect( mJobFilterEdit, SIGNAL( filterChanged( const Expression & ) ), SLOT( jobFilterChanged( const Expression & ) ) );

		connect( RefreshAction, SIGNAL( triggered(bool) ), SLOT( refresh() ) );

		connect( RestartAction, SIGNAL( triggered(bool) ), SLOT( restartJobs() ) );
		connect( ResumeAction, SIGNAL( triggered(bool) ), SLOT( resumeJobs() ) );
		connect( PauseAction, SIGNAL( triggered(bool) ), SLOT( pauseJobs() ) );
		connect( KillAction, SIGNAL( triggered(bool) ), SLOT( deleteJobs() ) );
		connect( ShowMineAction, SIGNAL( toggled(bool) ), SLOT( showMine(bool) ) );
		connect( WhoAmIAction, SIGNAL( triggered(bool) ), SLOT( whoAmI() ) );
		connect( FrameCyclerAction, SIGNAL( triggered(bool) ), SLOT( frameCycler() ) );
		connect( PdPlayerAction, SIGNAL( triggered(bool) ), SLOT( pdPlayer() ) );
		connect( ShowOutputAction, SIGNAL( triggered(bool) ), SLOT( outputPathExplorer() ) );
		connect( ClearErrorsAction, SIGNAL( triggered(bool) ), SLOT( clearErrors() ) );
		connect( ExploreJobFile, SIGNAL( triggered(bool) ), SLOT( exploreJobFile() ) );

		connect( mJobTree, SIGNAL( selectionChanged(RecordList) ), SLOT( jobListSelectionChanged() ) );
		connect( mJobTree, SIGNAL( currentChanged( const Record & ) ), SLOT( currentJobChanged() ) );
		connect( mJobTree, SIGNAL( expanded( const QModelIndex & ) ), SLOT( jobDependenciesExpanded( const QModelIndex & ) ) );

		connect( mFrameTree, SIGNAL( currentChanged( const Record & ) ), SLOT( frameSelected(const Record &) ) );
		connect( mFrameTree,  SIGNAL( selectionChanged(RecordList) ), SLOT( frameListSelectionChanged() ) );
		connect( mImageView, SIGNAL( frameStatusChange(int,int) ), SLOT( setFrameCacheStatus(int,int) ) );
		connect( mErrorTree, SIGNAL( selectionChanged(RecordList) ), SLOT( errorListSelectionChanged() ) );

		mJobTree->setContextMenuPolicy( Qt::CustomContextMenu );
		mFrameTree->setContextMenuPolicy( Qt::CustomContextMenu );
		mErrorTree->setContextMenuPolicy( Qt::CustomContextMenu );

		connect( mJobTree, SIGNAL( customContextMenuRequested( const QPoint & ) ), SLOT( showJobPopup( const QPoint & ) ) );
		connect( mFrameTree, SIGNAL( customContextMenuRequested( const QPoint & ) ), SLOT( showFramePopup( const QPoint & ) ) );
		connect( mErrorTree, SIGNAL( customContextMenuRequested( const QPoint & ) ), SLOT( showErrorPopup( const QPoint & ) ) );
		connect( mErrorTree, SIGNAL( aboutToShowHeaderMenu( QMenu * ) ), SLOT( populateErrorTreeHeaderMenu( QMenu * ) ) );

		mJobTabWidget->setTabText(0, "&Frames");
		mJobTabWidget->setTabText(1, "&Errors");
		mJobTabWidget->setTabText(2, "&History");
		mJobTabWidget->setTabText(4, "Job &Settings");
		connect( mJobTabWidget, SIGNAL( currentChanged( int ) ), SLOT( currentTabChanged() ) );
		connect( mFrameTabs, SIGNAL( currentChanged( int ) ), SLOT( frameTabChanged() ) );

		{
			JobModel * jm = new JobModel( mJobTree );
			jm->setAutoSort( true );
			mJobTree->setModel( jm );
			mJobTree->setItemDelegateForColumn( 2, new ProgressDelegate( mJobTree ) );
			mJobTree->setItemDelegateForColumn( 29, new JobIconDelegate( mJobTree ) );
			mJobTree->setDragEnabled( true );
			mJobTree->setAcceptDrops( true );
			mJobTree->setDropIndicatorShown(true);
			connect( jm, SIGNAL( dependencyAdded( const QModelIndex & ) ), mJobTree, SLOT( expand( const QModelIndex & ) ) );
		}

		{
			//mErrorTree->setItemDelegate( new MultiLineDelegate( mErrorTree ) );
			mErrorTree->setUniformRowHeights( false );
			JobErrorModel * em = new JobErrorModel( mErrorTree );
			em->setAutoSort( true );
			mErrorTree->setModel( em );
			for( int i=0; i < em->columnCount(); i++ )
				mErrorTree->setColumnAutoResize(i,true);
		}

		{
			RecordSuperModel * fm = new RecordSuperModel( mFrameTree );
			new FrameTranslator( fm->treeBuilder() );
			fm->setAutoSort( true );
			mFrameTree->setModel( fm );
			mFrameTree->setItemDelegateForColumn( 3, new LoadedDelegate( mJobTree ) );
			mFrameTree->setItemDelegateForColumn( 4, new LoadedDelegate( mJobTree ) );
			for(int i=0; i < fm->columnCount(); i++)
				mFrameTree->setColumnAutoResize( i, true );
		}
		mStatusFilterMenu = new StatusFilterMenu( this );
		mProjectFilterMenu = new ProjectFilterMenu( this );
		mJobTypeFilterMenu = new JobListJobTypeFilterMenu( this );
		mJobServiceFilterMenu = new JobServiceFilterMenu( this );

		jobListSelectionChanged();
		FreezerCore::addTask( new StaticJobListDataTask( this ) );
		
		setupJobView(mJobTree, ini);
		setupJobErrorView(mErrorTree, ini);
		setupFrameView(mFrameTree, ini);

		setDependencyTreeEnabled( ini.readBool( "DependencyTreeEnabled", false ), /* allowRefresh= */ false );

		FilterAction->setChecked( ini.readBool( "Filter", true ) );
		toggleFilter( FilterAction->isChecked() );

		mFrameTabs->setCurrentIndex( ini.readInt( "FrameTab", 0 ) );

		QStringList sl = ini.readString( "JobSplitterPos" ).split(',');
		QList<int> vl;
		foreach( QString s, sl )
			vl << s.toInt();
		
		// Reasonable defaults for the splitter
		if( vl.size()!=2 ) {
			vl.clear();
			int h = height();
			if( h < 300 )
				vl << h << 0;
			else
				vl << (int)(h * .6) << (int)(h * .4);
		}

		mJobSplitter->setSizes( vl );

		QStringList frameSplitterSizes = ini.readString( "FrameSplitterPos","750,1100" ).split(',');
		QList<int> frameSplitterInts;
		for( QStringList::Iterator it=frameSplitterSizes.begin(); it!=frameSplitterSizes.end(); ++it )
			frameSplitterInts += (*it).toInt();
		mFrameSplitter->setSizes( frameSplitterInts );

		// Filter any empty entries.  And empty string split with ',' returns a string list with one empty entry
		mJobFilter.typesToShow = ini.readUIntList( "TypeToShow" );
		mJobFilter.statusToShow = 	ini.readString( "StatusToShow", "submit,verify,ready,holding,started,suspended" ).split(',',QString::SkipEmptyParts);
		mJobFilter.userList = 		ini.readUIntList( "UserList" );
		ShowMineAction->blockSignals(true);
		ShowMineAction->setChecked( !mJobFilter.userList.isEmpty() );
		mShowingMine = ( !mJobFilter.userList.isEmpty() );
		ShowMineAction->blockSignals(false);
		mJobFilter.allProjectsShown = ini.readBool( "AllProjectsShown", false );
		if( ini.keys().contains( "VisibleProjects" ) ) {
			mJobFilter.visibleProjects = ini.readUIntList( "VisibleProjects" );
		} else {
			mJobFilter.hiddenProjects = ini.readUIntList( "HiddenProjects" );
			if( mJobFilter.hiddenProjects.isEmpty() )
				mJobFilter.allProjectsShown = true;
		}
		mJobFilter.showNonProjectJobs = ini.readBool( "ShowNonProjectJobs", true );
		mJobFilterEdit->lineEdit()->setText(ini.readString( "ExtraFilters", "" ));
		mJobFilter.mExtraFilters = mJobFilterEdit->expression();
		mJobFilter.mLimit = options.mLimit;
		mJobFilter.mDaysLimit = options.mDaysLimit;
		applyOptions();

		// load this once
		mMainUserList = Employee::select();

		// Don't show the panel if there's no plugins to begin with
		if( JobErrorsWidgetFactory::mJobErrorsWidgetPlugins.size() == 0 )
			mErrorPluginWidget->setVisible(false);
		else {
			// Initialise the widgets plugins in the errors tab
			foreach( JobErrorsWidgetPlugin * jewp, JobErrorsWidgetFactory::mJobErrorsWidgetPlugins.values() )
				jewp->initialize( mErrorPluginWidget );

			mErrorPluginWidget->setVisible(true);
		}
		
		// Initialise the frames tab plugins
		foreach( JobFramesTabWidgetPlugin * jftwp, JobFramesTabWidgetFactory::mJobFramesTabWidgetPlugins.values() )
			jftwp->initialize( mFrameTabs );
		
		// This only changes layout properties and hidden/shown mTabToolBar
		currentTabChanged( /*refresh=*/ false );
	}
}

void JobListWidget::save( IniConfig & ini, bool forceFullSave )
{
	// We use the viewConfig() for doing the actual restore now, since the ini
	// passed to this function may be a blank one used to make a copy of this view
	if( forceFullSave && !mViewsInitialized )
		initializeViews(viewConfig());
	
	if( mViewsInitialized ) {
		saveJobView(mJobTree,ini);
		saveJobErrorView(mErrorTree,ini);
		saveFrameView(mFrameTree,ini);
		ini.writeInt("FrameTab", mFrameTabs->currentIndex());
		ini.writeString( "StatusToShow", mJobFilter.statusToShow.join(",") );
		ini.writeUIntList( "UserList", mJobFilter.userList );
		ini.writeUIntList( "VisibleProjects", mJobFilter.visibleProjects );
		ini.removeKey( "HiddenProjects" );
		ini.writeBool( "AllProjectsShown", mJobFilter.allProjectsShown );
		ini.writeBool( "ShowNonProjectJobs", mJobFilter.showNonProjectJobs );
		ini.writeString( "ExtraFilters", mJobFilterEdit->lineEdit()->text() );
		ini.writeUIntList( "TypeToShow", mJobFilter.typesToShow );
		ini.writeUIntList( "TypesHidden", (mSharedData->mJobTypeList - JobType::table()->records( mJobFilter.typesToShow )).keys() );
		ini.writeBool( "DependencyTreeEnabled", isDependencyTreeEnabled() );
		ini.writeBool( "Filter", FilterAction->isChecked() );
		// Save the splitter position by making a string of ints separated by commas
		// Output string
		QString jsps;
		// Use comma
		QList<int> list = mJobSplitter->sizes();
		// Stupid qsplitter doesn't return proper sizes if this tab hasn't been shown
		// It returns 12/12 or 13/13 in that case so we won't save
		if( list.size() == 2 && !(list[0] == list[1] && list[0] < 20)) {
			QStringList sizes;
			foreach( int i, list ) sizes += QString::number(i);
			ini.writeString( "JobSplitterPos", sizes.join(",") );
		}
		list = mFrameSplitter->sizes();
		// Stupid qsplitter doesn't return proper sizes if this tab hasn't been shown
		// It returns 12/12 or 13/13 in that case so we won't save
		if( list.size() == 2 && !(list[0] == list[1] && list[0] < 20)) {
			QStringList sizes;
			foreach( int i, list ) sizes += QString::number(i);
			ini.writeString( "TaskImageSplitterPos", sizes.join(",") );
		}
	}
	FreezerView::save(ini,forceFullSave);
}

void JobListWidget::restore( IniConfig & ini, bool forceFullRestore )
{
	FreezerView::restore(ini,forceFullRestore);
	if( forceFullRestore )
		initializeViews(ini);
}

ProjectList JobListWidget::activeProjects()
{
	return mSharedData ? mSharedData->mProjectList : ProjectList();
}

JobTypeList JobListWidget::activeJobTypes()
{
	return mSharedData ? mSharedData->mJobTypeList : JobTypeList();
}

ServiceList JobListWidget::activeServices()
{
	return mSharedData ? mSharedData->mServiceList : ServiceList();
}

bool JobListWidget::isDependencyTreeEnabled() const
{
	return ((JobModel*)mJobTree->model())->isDependencyTreeEnabled();
}

void JobListWidget::setDependencyTreeEnabled( bool dte, bool allowRefresh )
{
	if( dte != isDependencyTreeEnabled() ) {
		((JobModel*)mJobTree->model())->setDependencyTreeEnabled( dte );
		DependencyTreeEnabledAction->setChecked( dte );
		mJobTree->setRootIsDecorated( dte );
		if( allowRefresh && dte ) refresh();
	}
}

void JobListWidget::jobDependenciesExpanded( const QModelIndex & idx )
{
	if( !idx.parent().isValid() )
		mJobTree->expandRecursive( idx );
}

bool JobListWidget::event( QEvent * event )
{
	if( event->type() == QEvent::Show ) {
		initializeViews(viewConfig());
	}
	return QWidget::event(event);
}

void JobListWidget::customEvent( QEvent * evt )
{
	switch( (int)evt->type() ) {
		case JOB_LIST:
		{
			mJobTask = ((JobListTask*)evt);
			JobModel * jm = (JobModel*)mJobTree->model();
			JobList existing, toAdd;
			QModelIndexList toRemove;
			QMap<int,QPair<QModelIndex,bool> > existingMap;
			
			QTime t;
			t.start();
			JobList topLevelJobs = mJobTask->mReturn - mJobTask->mDependentJobs;
			LOG_5( QString("Took %1 ms to subtract dependant jobs. %2 top level jobs").arg(t.elapsed()).arg(topLevelJobs.size()) );
			
			t.start();
			ModelGrouper * grouper = jm->grouper();
			int topLevelDepth = 0;
			if( grouper && grouper->isGrouped() )
				topLevelDepth = 1;

			for( ModelIter it(jm,ModelIter::Filter(ModelIter::Recursive|ModelIter::DescendLoadedOnly)); it.isValid(); ++it ) {
				QModelIndex idx = *it;
				if( topLevelDepth == it.depth() ) {
					Job j = jm->getRecord(idx);
					existingMap[j.key()] = qMakePair<QModelIndex,bool>(idx,false);
				}
			}
			LOG_5( QString("Took %1 ms to map existing jobs, %2 jobs mapped").arg(t.elapsed()).arg(existingMap.size()) );
			t.start();
			
			foreach( Job j, topLevelJobs ) {
				QMap<int,QPair<QModelIndex,bool> >::Iterator it = existingMap.find(j.key());
				if( it != existingMap.end() ) {
					existing += j;
					jm->updateIndex(it.value().first);
					it.value().second = true;
				} else
					toAdd += j;
			}
			LOG_5( QString("Took %1 ms to update existing indexes").arg(t.elapsed()) );
			t.start();
			
			for( QMap<int,QPair<QModelIndex,bool> >::Iterator it = existingMap.begin(); it != existingMap.end(); ++it )
				if( !it.value().second )
					toRemove += it.value().first;
			
			LOG_5( "Removing " + QString::number(toRemove.size()) + " jobs" );
			jm->remove(toRemove);
			LOG_5( QString("Took %1 ms to remove old jobs").arg(t.elapsed()) );
			LOG_5( "Appending " + QString::number(toAdd.size()) + " jobs to the list" );
			t.start();
			
			jm->append( toAdd );
			LOG_5( QString("Took %1 ms to append new jobs").arg(t.elapsed()) );
			t.start();
			
			QMap<Record, JobServiceList> jobServicesByJob;
			if( mJobTask->mFetchJobServices ) {
				jobServicesByJob = mJobTask->mJobServices.groupedBy<Record,JobServiceList,uint,Job>( "fkeyjob" );
				//LOG_5( QString("Got %1 services for %2 jobs").arg(mJobTask->mJobServices.size()).arg(jobServicesByJob.size()) );
			}

			LOG_5( QString("Took %1 ms to group services").arg(t.elapsed()) );
			t.start();

			QMap<uint,JobDepList> jobDepsByJob = mJobTask->mJobDeps.groupedBy<uint,JobDepList>("fkeyjob");

			LOG_5( QString("Took %1 ms to group deps").arg(t.elapsed()) );
			t.start();

			for( ModelIter it(jm,ModelIter::Filter(ModelIter::Recursive|ModelIter::DescendLoadedOnly)); it.isValid(); ++it ) {
				Job j = jm->getRecord(*it);
				if( j.isRecord() ) {
					JobDepList deps = jobDepsByJob[j.key()];
					if( deps.size() )
						jm->updateRecords( deps.deps(), *it, false );
					if( mJobTask->mFetchJobServices ) {
						// Update services
						JobItem & ji = JobTranslator::data(*it);
						QMap<Record,JobServiceList>::iterator jsit = jobServicesByJob.find(ji.job);
						if( jsit != jobServicesByJob.end() )
							ji.services = jsit.value().services().services().join(",");
					}
				}
			}
			LOG_5( QString("Took %1 ms to setup deps and services").arg(t.elapsed()) );

			t.start();
			// clear out existing toolTip info first
			clearChildrenToolTip(mJobTree->rootIndex());

			// recursively set all rows
			//QTimer::singleShot(20, this, SLOT(setToolTips()));
			setToolTips();
			LOG_5( QString("Took %1 ms to set tooltips").arg(t.elapsed()) );

			mJobTree->busyWidget()->stop();
			mJobTaskRunning = false;
			// This will update the status bar and action states
			// since they selected jobs may have changed
			jobListSelectionChanged();

			/*
			if( mQueuedJobRefresh ) {
				mQueuedJobRefresh = false;
				refresh();
			}
			*/
			t.start();
			mJobTree->mRecordFilterWidget->filterRows();
			LOG_5( QString("Took %1 ms to filter rows").arg(t.elapsed()) );
			break;
		}
		case FRAME_LIST:
		{
			int minFrame = -1, maxFrame = -1;
			JobTaskList jtl = ((FrameListTask*)evt)->mReturn;
			FrameItem::CurTime = ((FrameListTask*)evt)->mCurTime;

			mFrameTree->model()->updateRecords( jtl );
			
			if( mFrameTabs->currentWidget() == mFrameGraphTab ) {
				refreshGraphsTab(jtl);
			}
			else if( mFrameTabs->currentWidget() == mFrameImageTab ) {
				foreach( JobTask jt, jtl )
				{
					if( minFrame==-1 || (int)jt.frameNumber() < minFrame )
						minFrame = jt.frameNumber();
					if( maxFrame==-1 || (int)jt.frameNumber() > maxFrame )
						maxFrame = jt.frameNumber();
				}
				//mTabToolBar->slotPause();
				mImageView->setFrameRange( mCurrentJob.outputPath(), minFrame, maxFrame, true );
				mFrameTask = 0;
			}
			else if( mFrameTabs->currentWidget() == mFrameDepsTab ) {
				refreshDepsTab();
			}

			mFrameTree->mRecordFilterWidget->filterRows();
			mFrameTree->busyWidget()->stop();

			mTabToolBar->slotPause();
			break;
		}
		case PARTIAL_FRAME_LIST:
		{
			JobTaskList jtl = ((PartialFrameListTask*)evt)->mReturn;
			//LOG_3("got partial frame list back:"+QString::number(jtl.size()));
			FrameItem::CurTime = ((PartialFrameListTask*)evt)->mCurTime;
			mFrameTree->model()->updated( jtl );
			//mTabToolBar->slotPause();
			mFrameTree->mRecordFilterWidget->filterRows();
			mFrameTree->busyWidget()->stop();
			mPartialFrameTask = 0;
			break;
		}
		case ERROR_LIST:
		{
			JobErrorList jer = ((ErrorListTask*)evt)->mReturn;
			mErrorTree->busyWidget()->stop();
			mErrorTree->model()->updateRecords(jer);
			//mErrorTree->mRecordFilterWidget->filterRows();
			break;
		}
		case STATIC_JOB_LIST_DATA:
		{
			mStaticDataRetrieved = true;
			StaticJobListDataTask * sdt = (StaticJobListDataTask*)evt;

			// Only the first StaticJobListDataTask actual does the select and fills in these static structures
			// any subsequent ones just ensure that the data is retreived before doing the rest of this logic
			if( sdt->mHasData ) {
				mSharedData->mJobTypeList = sdt->mJobTypes;
				mSharedData->mProjectList = sdt->mProjects;
				//mSharedData->mServiceList = sdt->mServices;
			}

			// Default to showing all of the services and job types
			IniConfig & ini = viewConfig();
			
			if( mJobFilter.typesToShow.isEmpty() )
				// For now only filter by primary job types, not sub-jobtypes
				mJobFilter.typesToShow = mSharedData->mJobTypeList.filter( "fkeyparentjobtype", QVariant(QVariant::Int), false ).keys();
			else {
				QList<uint> verifiedKeyList = verifyKeyList( mJobFilter.typesToShow, JobType::table() );
				JobTypeList toShow = JobType::table()->records( verifiedKeyList );
				if( ini.keys().contains( "TypesHidden" ) ) {
					JobTypeList hidden = JobType::table()->records( ini.readUIntList( "TypesHidden" ) );
					JobTypeList newTypes = mSharedData->mJobTypeList - hidden - toShow;
					if( newTypes.size() && toShow.size() > 1 )
						toShow += newTypes;
				} else {
					QStringList toShowNames = toShow.names();
					if( toShowNames.contains( "Max2009" ) && !toShowNames.contains( "Max2010" ) )
						toShow += JobType::recordByName( "Max2010" );
				}
				mJobFilter.typesToShow = toShow.unique().keys();
			}
			
			if( mJobFilter.servicesToShow.isEmpty() )
				// For now only filter by primary job types, not sub-jobtypes
				mJobFilter.servicesToShow = mSharedData->mServiceList.keys();
			else
				mJobFilter.servicesToShow = verifyKeyList( mJobFilter.servicesToShow, Service::table() );
				
			if( mJobFilter.hiddenProjects.size() ) {
				mJobFilter.visibleProjects = mSharedData->mProjectList.keys();
				foreach( uint hidden, mJobFilter.hiddenProjects )
					mJobFilter.visibleProjects.removeAll( hidden );
				mJobFilter.hiddenProjects.clear();
			}

			// If we haven't retrieved the static data, then mJobTaskRunning indicates
			// that we need to refresh the job list.
			if( mJobTaskRunning ) {
				mJobTaskRunning = false;
				refresh();
			}
			break;
		}
		case JOB_HISTORY_LIST:
		{
			mHistoryView->setHistory( ((JobHistoryListTask*)evt)->mReturn );
			mHistoryView->busyWidget()->stop();
		}
		case UPDATE_JOB_LIST:
		{
			JobList jl = ((UpdateJobListTask*)evt)->mReturn;
			mJobTree->model()->updated(jl);

			mJobTree->mRecordFilterWidget->filterRows();

			break;
		}
		default:
			break;
	}
}

void JobListWidget::refreshGraphsTab(JobTaskList jtl) const
{
	int minFrame = -1, maxFrame = -1;
	jtl.jobTaskAssignments().jobAssignments();
	QMap<QString, QString> stats;
	foreach( JobTask jt, jtl )
	{
		if( minFrame==-1 || (int)jt.frameNumber() < minFrame )
			minFrame = jt.frameNumber();
		if( maxFrame==-1 || (int)jt.frameNumber() > maxFrame )
			maxFrame = jt.frameNumber();

		// If there is no end time use now
		QDateTime end = jt.jobTaskAssignment().ended().isNull() ? QDateTime::currentDateTime() : jt.jobTaskAssignment().ended();

		// Store the frame time if the task has started
		int time = jt.jobTaskAssignment().started().isNull() ? 0 : jt.jobTaskAssignment().started().secsTo(end);

		// Get Frame specific stats
		QString frameNumber     = QString::number( (int)jt.frameNumber() );
		QString frameMemory     = QString::number( jt.jobTaskAssignment().memory() / 1024 ); // MB
		QString frameCpuTime    = QString::number( time * jt.jobTaskAssignment().jobAssignment().assignSlots() / 60 ); // Minutes
		QString frameWallTime   = QString::number( time / 60 );
		QString frameOpsRead    = QString::number( jt.jobTaskAssignment().jobAssignment().opsRead() );
		QString frameOpsWrite   = QString::number( jt.jobTaskAssignment().jobAssignment().opsWrite() );
		QString frameBytesRead  = QString::number( jt.jobTaskAssignment().jobAssignment().bytesRead() / 1024 ); // MB
		QString frameBytesWrite = QString::number( jt.jobTaskAssignment().jobAssignment().bytesWrite() / 1024 );

		// Fill the stats map
		stats["memory_"     + jt.status()] += "[" + frameNumber + ","  + frameMemory     + "],";
		stats["cputime_"    + jt.status()] += "[" + frameNumber + ","  + frameCpuTime    + "],";
		stats["walltime_"   + jt.status()] += "[" + frameNumber + ","  + frameWallTime   + "],";
		stats["opsread_"    + jt.status()] += "[" + frameNumber + ","  + frameOpsRead    + "],";
		stats["opswrite_"   + jt.status()] += "[" + frameNumber + ",-" + frameOpsWrite   + "],";
		stats["bytesread_"  + jt.status()] += "[" + frameNumber + ","  + frameBytesRead  + "],";
		stats["byteswrite_" + jt.status()] += "[" + frameNumber + ",-" + frameBytesWrite + "],";
	}

	QString params = "{";
	QMapIterator<QString, QString> stat(stats);
	while (stat.hasNext()) {
		stat.next();

		params += "'" + stat.key() + "': " + "[" + stat.value() + "],";
	}
	params += "}";

	// Plot the memory/time graphs
	mGraphs->page()->mainFrame()->evaluateJavaScript(QString("plot(%1)").arg(params));
}

QMap<QString,QString> JobListWidget::userToolTipMap() const
{
	QMap<QString,QString> userToolTips;
	// Update slot use data for User tooltips
	if( mJobTask->mFetchUserServices ) {
		// we store the pre-formatted tooltip in the model so
		// need to do that now..
		foreach( QString key, mJobTask->mUserServiceCurrent.keys() ) {
			QString keyUser = key.section(":",0,0);
			QString keyService = key.section(":",1,1);
			QString toolTip = QString("%1 : %2").arg(keyService).arg(QString::number(mJobTask->mUserServiceCurrent[key]));
			if( mJobTask->mUserServiceLimits.contains(key) )
				toolTip += " / " + QString::number(mJobTask->mUserServiceLimits[key]);

			toolTip += "\n";
			userToolTips[keyUser] += toolTip;
		}
	}
	return userToolTips;
}

QMap<QString,QString> JobListWidget::projectToolTipMap() const
{
	QMap<QString,QString> projectToolTips;
	// Update slot use data for Project tooltips
	if( mJobTask->mFetchProjectSlots ) {
		// we store the pre-formatted tooltip in the model so
		// need to do that now..
		foreach( QString keyProject, mJobTask->mProjectSlots.keys() ) {
			QString value = mJobTask->mProjectSlots[keyProject];
			int projectCurrent = value.section(":",0,0).toInt();
			int projectReserve = value.section(":",1,1).toInt();
			int projectLimit = value.section(":",2,2).toInt();
			QString toolTip = QString("(%1)\n%2").arg(projectReserve).arg(projectCurrent);
			if( projectLimit > -1 )
				toolTip += " / " + QString::number(projectLimit);
			projectToolTips[keyProject] = toolTip;
		}
	}
	return projectToolTips;
}

void JobListWidget::clearChildrenToolTip( const QModelIndex & parent )
{
	JobModel * jm = (JobModel *)(mJobTree->model());
	int numRows = jm->rowCount(parent);
	for ( int row = 0; row < numRows; row++ ) {
		QModelIndex qmi = jm->index(row, 0, parent);
		Job j = jm->getRecord(qmi);
		if( !j.isRecord() )
			continue;
		JobItem & ji = JobTranslator::data(qmi);
		ji.userToolTip.clear();
		ji.projectToolTip.clear();

		if( jm->hasChildren(qmi) )
			clearChildrenToolTip(qmi);
	}
}

void JobListWidget::setToolTips()
{
	QMap<QString,QString> userTips = userToolTipMap();
	QMap<QString,QString> projectTips = projectToolTipMap();
	setChildrenToolTip( mJobTree->rootIndex(), userTips, projectTips );
}

void JobListWidget::setChildrenToolTip( const QModelIndex & parent, const QMap<QString,QString> & userToolTips, const QMap<QString,QString> & projectToolTips )
{
	JobModel * jm = (JobModel *)(mJobTree->model());
	int numRows = jm->rowCount(parent);
	for ( int row = 0; row < numRows; row++ ) {
		QModelIndex qmi = jm->index(row, 0, parent);
		Job j = jm->getRecord(qmi);
		if( !j.isRecord() )
			continue;

		// set user tool tip
		QModelIndex userIndex = jm->index(row, 4, parent);
		QString userCell = jm->data( userIndex ).toString();
		//LOG_3( QString("row %1 has data %2").arg(QString::number(qmi.row())).arg(cell) );
		if( userToolTips.contains(userCell) ) {
			JobItem & ji = JobTranslator::data(userIndex);
			ji.userToolTip += userToolTips[userCell];
		}

		//set project tooltip
		QModelIndex projectIndex = jm->index(row, 7);
		QString projectCell = jm->data( projectIndex ).toString();
		//LOG_3( QString("row %1 has data %2").arg(QString::number(qmi.row())).arg(cell) );
		if( projectToolTips.contains(projectCell) ) {
			JobItem & ji = JobTranslator::data(projectIndex);
			ji.projectToolTip += projectToolTips[projectCell];
		}

		if( jm->hasChildren(qmi) )
			setChildrenToolTip(qmi, userToolTips, projectToolTips);
	}
}


QToolBar * JobListWidget::toolBar( QMainWindow * mw )
{
	if( !mToolBar ) {
		User currentUser = User::currentUser();
		initializeViews(viewConfig());
		mToolBar = new QToolBar( mw );
		mToolBar->addAction( RefreshAction );
		mToolBar->addAction( FrameCyclerAction );
		//mToolBar->addAction( VNCHostsAction );
		mToolBar->addSeparator();
		mToolBar->addAction( ResumeAction );
		mToolBar->addAction( PauseAction );
		mToolBar->addAction( KillAction );
		mToolBar->addAction( RestartAction );
		mToolBar->addSeparator();
		mToolBar->addAction( FilterAction );
		mToolBar->addAction( FilterClearAction );
		mToolBar->addAction( ShowMineAction );

		if( currentUser.isRecord() ) {
			UserGroupList ugl = currentUser.userGroups();
			foreach( UserGroup ug, ugl)
				if( ug.group() == Group::recordByName("RenderOps") ) {
					mToolBar->addAction( WhoAmIAction );
					break;
				}
		}

		mToolBar->addSeparator();
		mToolBar->addAction( DependencyTreeEnabledAction );
		mToolBar->addWidget( mJobFilterEdit );
	}
	return mToolBar;
}

void JobListWidget::populateViewMenu( QMenu * viewMenu )
{
	User currentUser = User::currentUser();
	initializeViews(viewConfig());
	viewMenu->addAction( DependencyTreeEnabledAction );
	viewMenu->addAction( NewViewFromSelectionAction );
	viewMenu->addSeparator();
	viewMenu->addAction( ShowMineAction );

	viewMenu->addAction( FilterAction );
	viewMenu->addAction( FilterClearAction );
	{
		QMenu * filterMenu = viewMenu->addMenu( "Job Filters" );
		filterMenu->addMenu( mProjectFilterMenu );
		filterMenu->addMenu( mStatusFilterMenu );
		filterMenu->addMenu( mJobTypeFilterMenu );
		filterMenu->addMenu( mJobServiceFilterMenu );
	}
}

void JobListWidget::setLimit()
{
	bool ok;
	int limit = QInputDialog::getInteger( this, "Set Job Limit", "Enter Maximum number of jobs to display", options.mLimit, 1, 100000, 1, &ok );
	if( ok ) {
		options.mLimit = limit;
		applyOptions();
		refresh();
	}
}

void JobListWidget::applyOptions()
{
	if( mViewsInitialized ) {
		mJobFilter.mLimit = options.mLimit;
		mJobFilter.mDaysLimit = options.mDaysLimit;
		mJobTree->setFont( options.jobFont );
		mJobTree->setDragEnabled( !options.mControlModifierDepDragCheck );
		mFrameTree->setFont( options.frameFont );
		mErrorTree->setFont( options.frameFont );
		mSummaryTab->setFont( options.summaryFont );
		options.mJobColors->apply(mJobTree);
		options.mFrameColors->apply(mFrameTree);
		options.mErrorColors->apply(mErrorTree);

		QPalette p = mJobTree->palette();
		ColorOption * co = options.mJobColors->getColorOption("default");
		p.setColor(QPalette::Active, QPalette::AlternateBase, co->bg.darker(120));
		p.setColor(QPalette::Inactive, QPalette::AlternateBase, co->bg.darker(120));
		mJobTree->setPalette( p );

		mHistoryView->applyOptions();
		mJobTree->update();
		mFrameTree->update();
		mErrorTree->update();
	}
}

void JobListWidget::setJobFilter( const JobFilter & jf )
{
	mJobFilter = jf;
}

void JobListWidget::setHiddenProjects( ProjectList hiddenProjects )
{
	mJobFilter.hiddenProjects = hiddenProjects.keys();
}

void JobListWidget::setElementList( ElementList el )
{
	mJobFilter.elementList = el;
}

void JobListWidget::setStatusToShow( QStringList statii )
{
	mJobFilter.statusToShow = statii;
}

void JobListWidget::doRefresh()
{
	FreezerView::doRefresh();
	bool needJobListTask = false;
	bool needStatusBarMsg = false;

	if( !mStaticDataRetrieved )
		mJobTaskRunning = needStatusBarMsg = true;
	else if( mJobTaskRunning )
		mQueuedJobRefresh = true;
	else
		needJobListTask = needStatusBarMsg = true;

	if( needStatusBarMsg )
		setStatusBarMessage( "Refreshing Job List..." );

	if( needJobListTask ) {
		mJobTaskRunning = true;
		LOG_5( "Statuses to show: "	+ mJobFilter.statusToShow.join(",") );
		LOG_5( "Types to show: " + JobTypeList(JobType::table()->records(mJobFilter.typesToShow)).names().join(",") );
		mJobTree->busyWidget()->start();
		FreezerCore::addTask( new JobListTask( this, mJobFilter, mJobList, activeProjects(), !mJobTree->isColumnHidden(19) /*Service column*/, isDependencyTreeEnabled()) );
		FreezerCore::wakeup();
		// Refresh frame or error list too
		currentJobChanged();
	}
}

void JobListWidget::jobTreeColumnVisibilityChanged( int column, bool visible )
{
	// Services column
	if( column == 19 && visible )
		refresh();
}

void JobListWidget::setJobList( JobList jobList )
{
	mJobList = jobList;
	refresh();
}

void JobListWidget::refreshErrorList()
{
	mErrorTree->busyWidget()->start();
	FreezerCore::addTask( new ErrorListTask( this, mCurrentJob, mShowClearedErrors ) );
	FreezerCore::wakeup();
}

void JobListWidget::currentJobChanged()
{
	bool jobChange = mCurrentJob != mJobTree->current();
	mCurrentJob = mJobTree->current();
	LOG_5( "JobListWidget::currentJobChanged: " + QString::number( mCurrentJob.key() ) );

	QWidget * curTab = mJobTabWidget->currentWidget();
	if( curTab==mFrameTab ) {
		refreshFrameList(jobChange);
		if( JobFramesTabWidgetFactory::mJobFramesTabWidgetPlugins.size() ) {
			JobList jList = mJobTree->selection();

			foreach( JobFramesTabWidgetPlugin * jftwp, JobFramesTabWidgetFactory::mJobFramesTabWidgetPlugins.values() )
				jftwp->setJobList( jList, mFrameTabs->currentIndex() );
		}
    }
	else if( curTab == mErrorsTab )
		refreshErrorList();
}

void JobListWidget::refreshFrameList( bool jobChange )
{
	LOG_3(QString("refreshFrameList: jobChange: %1").arg(jobChange));
	// Cancel the current task if there is one
	//if( mFrameTask )
	//	mFrameTask->mCancel = true;
	
	if( jobChange )
		mFrameTree->model()->setRootList( RecordList() );

	if( mCurrentJob.isRecord() ) {
		if( jobChange || mCurrentJob.jobStatus().tasksCount() < 2000 ) {
			mFrameTask = new FrameListTask( this, mCurrentJob );
			mFrameTree->busyWidget()->start();
			FreezerCore::addTask( mFrameTask );
			FreezerCore::wakeup();
		} else {
			// only update visible items
			QModelIndex start = mFrameTree->indexAt(QPoint(1,1));
			QModelIndex end = mFrameTree->indexAt(QPoint( 1, mFrameTree->viewport()->height()+1 ));
			JobTaskList jtl;
			for(ModelIter it(start); *it != end; ++it)
				jtl += FrameTranslator::getRecordStatic(*it);
			mPartialFrameTask = new PartialFrameListTask( this, jtl );
			mFrameTree->busyWidget()->start();
			FreezerCore::addTask( mPartialFrameTask );
			FreezerCore::wakeup();
		}
	}
}

void JobListWidget::jobListSelectionChanged()
{
	JobList selection = mJobTree->selection();
	if( selection.size() == 1 ) {
		setStatusBarMessage( selection[0].name() + " Selected" );
	} else if( selection.size() )
		setStatusBarMessage( QString::number( selection.size() ) + " Jobs Selected" );
	else
		clearStatusBar();
	
	mSelectedJobs = selection;
	LOG_5( "Selection is " + mSelectedJobs.keyString() + " -- Current is " + QString::number( mCurrentJob.key() ) );

	QWidget * curTab = mJobTabWidget->currentWidget();
	if( curTab==mSummaryTab ) {
		mJobSettingsWidget->setSelectedJobs( selection );
	} else if( curTab == mHistoryTab ) {
		// Clear the view
		mHistoryView->setHistory( JobHistoryList() );
		mHistoryView->busyWidget()->start();
		FreezerCore::addTask( new JobHistoryListTask( this, selection ) );
		FreezerCore::wakeup();
	} else if( curTab == mNotesTab ) {
		mThreadView->setJobList( selection );
	}
	
	int cnt = mSelectedJobs.size();
	QMap<QString,RecordList> byStatus = mSelectedJobs.groupedBy( "status" );
	int newCnt = byStatus["submit"].size() + byStatus["verify"].size();
	int activeCnt = byStatus["ready"].size() + byStatus["started"].size();
	int doneCnt = byStatus["done"].size();
	int suspCnt = byStatus["suspended"].size();
	int holdCnt = byStatus["holding"].size();
	
	// Allow resume if all selected jobs are suspended
	ResumeAction->setEnabled( cnt && (suspCnt == cnt) );
	// Allow pause if all selected jobs are active( ready/started )
	PauseAction->setEnabled( cnt && (activeCnt + holdCnt + suspCnt == cnt) );
	// Allow Remove if all selected jobs are active, done, or suspended
	bool allowDelete = cnt && (activeCnt + doneCnt + suspCnt + newCnt + holdCnt == cnt);
	// Permission check.  Only allow deleting a user's own jobs unless they have permission to modify jobs
	//if( allowDelete )
	//	allowDelete &= (mSelectedJobs.users().unique() == RecordList(User::currentUser()) || User::hasPerms( "Job", true ));
	KillAction->setEnabled( allowDelete );
	// Allow Restart if all selected jobs are active, done, or suspended
	RestartAction->setEnabled( cnt && (activeCnt + doneCnt + suspCnt == cnt) );
}

void JobListWidget::refreshCurrentTab()
{
	QWidget * curTab = mJobTabWidget->currentWidget();
	if( curTab==mSummaryTab || curTab == mHistoryTab )
		jobListSelectionChanged();
	else if( curTab==mFrameTab )
		refreshFrameList(true);
	else if( curTab==mNotesTab )
		mThreadView->setJobList( mJobTree->selection() );
	else
		currentJobChanged();
}

void JobListWidget::currentTabChanged(bool refresh)
{
	QWidget * curTab = mJobTabWidget->currentWidget();

	// This hack allows the bottom part of the splitter to be smaller than
	// the SummaryTab's minimum height, when the SummaryTab is not shown
	if( curTab == mSummaryTab )
		mJobTabWidget->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );
	else
		mJobTabWidget->setSizePolicy( QSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored ) );

//	if( curTab==mFrameTab )
//		mTabToolBar->show();
//	else
//		mTabToolBar->hide();

	if( refresh )
		refreshCurrentTab();
}

void JobListWidget::frameTabChanged()
{
	refreshFrameList(false /*jobChanged*/);
}

void JobListWidget::errorListSelectionChanged()
{
	if( JobErrorsWidgetFactory::mJobErrorsWidgetPlugins.size() )
	{
		JobErrorList errors = mErrorTree->selection();

		foreach( JobErrorsWidgetPlugin * jewp, JobErrorsWidgetFactory::mJobErrorsWidgetPlugins.values() )
			jewp->setJobErrors( errors );
	}
}

void JobListWidget::clearErrors()
{
	JobList jobs = mJobTree->selection();
	if( jobs.size() ) {
		QString history = " Errors cleared";
		QSqlQuery q = Database::current()->exec("SELECT fkeyjob, sum(count) FROM JobError WHERE (cleared=false OR cleared IS NULL) AND fkeyjob IN (" + jobs.keyString() + ") GROUP BY fkeyjob");
	
		while( q.next() ) {
			Job j( q.value(0).toInt() );
			j.addHistory( QString::number(q.value(1).toInt()) + history );
		}
		Database::current()->exec("UPDATE JobError SET cleared=true WHERE fkeyJob IN(" + jobs.keyString() + ")");
	}
}

void JobListWidget::restartJobs()
{
	if( !(qApp->keyboardModifiers() & (Qt::ControlModifier|Qt::AltModifier)) && (QMessageBox::warning( this, "Restart Job Confirmation", "Are you sure that you want to restart the selected jobs?\n  All tasks will be set to 'new'.", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) )
		!= QMessageBox::Yes )
		return;
	updateSelectedJobs("verify", true);

	foreach( Job job, mJobTree->selection() )
		job.addHistory( "Job Restarted" );
}

void JobListWidget::resumeJobs()
{
	updateSelectedJobs("started", false);
	refresh();
}

void JobListWidget::pauseJobs()
{
	updateSelectedJobs("suspended", false);
	refresh();
}

void JobListWidget::deleteJobs()
{
	QString deleteStatus = "deleted";
	JobTypeList jobTypes = JobList(mJobTree->selection()).jobTypes().unique();
	// Give Archive option for batch jobs
	if( jobTypes.size() == 1 && jobTypes[0].name() == "Batch" ) {
		QMessageBox * mb = new QMessageBox(this);
		mb->setWindowTitle( "Archive or Delete Job Confirmation" );
		mb->setText( "You have the option to Archive or Delete the selected Batch Jobs.\n  Archived jobs will remain viewable, including their tasks, errors, and history." );
		mb->setDefaultButton( mb->addButton( "Archive", QMessageBox::AcceptRole ) );
		mb->addButton( "Delete", QMessageBox::DestructiveRole );
		mb->addButton( "Cancel", QMessageBox::RejectRole );
		mb->exec();
		int role =  mb->buttonRole(mb->clickedButton());
		delete mb;
		if( role == QMessageBox::RejectRole )
			return;
		if( role == QMessageBox::AcceptRole )
			deleteStatus = "archived";
	} else if( !(qApp->keyboardModifiers() & (Qt::ControlModifier|Qt::AltModifier)) && (QMessageBox::warning( this, "Delete Job Confirmation", "Are you sure that you want to delete the selected jobs?\n  Once deleted a job cannot be restarted.", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) )
		!= QMessageBox::Yes )
		return;
	
	updateSelectedJobs(deleteStatus, false);
	refresh();
}

void JobListWidget::updateSelectedJobs(const QString & jobStatus, bool resetTasks)
{
	JobList jl = mJobTree->selection(), checked;

	JobList holdingJobs;
	JobDepList depsToDelete;
	foreach( Job j, jl ) {
		if( jobStatus!="started" || j.status()=="suspended" )
			checked += j;
	}

	Job::updateJobStatuses( checked, jobStatus, resetTasks );
}

void JobListWidget::showMine(bool sm)
{
	User u = User::currentUser();

	/* Edit the mJobFilter */
	mJobFilter.userList.clear();
	mShowingMine = sm;
	if( sm && u.isRecord() )
		mJobFilter.userList += u.key();

	/* Refresh the list */
	refresh();
}

void JobListWidget::whoAmI()
{
	if( !mShowingMine )
		return;

	User u = User::currentUser();
	mJobFilter.userList.clear();

	if( u.isRecord() && !mCurrentlyImmitating.contains(u) )
		mCurrentlyImmitating += u;

	UserNotifyDialog und(this);
	und.setMainUserList(mMainUserList);
	und.setUsers(mCurrentlyImmitating);
	if( und.exec() == QDialog::Accepted ) {
		foreach( User su, und.userList() )
			mJobFilter.userList += su.key();
		mCurrentlyImmitating = und.userList();
	}

	// Make sure the current user is always included even if they delete their name off the list. Prevents the filter from just selecting everyone
	if( u.isRecord() && !mJobFilter.userList.contains(u.key()) )
		mJobFilter.userList += u.key();

	refresh();
}


void JobListWidget::jobFilterChanged( const Expression & jobFilter )
{
	mJobFilter.mExtraFilters = jobFilter;
	refresh();
}

void JobListWidget::frameSelected( const Record & frameRecord )
{
	JobTask frame(frameRecord);
	if( frame.isRecord() ) {
		int frameNumber = frame.frameNumber();
		mImageView->setImageNumber( frameNumber );
	}
}

void JobListWidget::frameListSelectionChanged()
{
	JobTaskList jtl = mFrameTree->selection();
	if( jtl.size() == 1 ) {
		int num = jtl[0].frameNumber();
		setStatusBarMessage( mCurrentJob.name() + " Frame " + QString::number( num ) );
	} else if( jtl.size() > 1 ) {
		setStatusBarMessage( mCurrentJob.name() + " " + QString::number( jtl.size() ) + " Frames Selected" );
	} else
		clearStatusBar();
	
	if( JobFramesTabWidgetFactory::mJobFramesTabWidgetPlugins.size() ) {
		foreach( JobFramesTabWidgetPlugin * jftwp, JobFramesTabWidgetFactory::mJobFramesTabWidgetPlugins.values() )
			jftwp->setJobTaskList( jtl, mFrameTabs->currentIndex() );
	}
}

void JobListWidget::frameCycler()
{
	if( mCurrentJob.isRecord() ) {
		// We should know (per jobtype?) whether the output path is a frame sequence or not
		// Could check too since we can stat the files if frame cycler can
		JobTask cur = mFrameTree->current();
		QString path = mCurrentJob.outputPath();
		int frameNumber = cur.isRecord() ? cur.frameNumber() : mCurrentJob.minTaskNumber();
		if( frameNumber >= 0 )
			path = makeFramePath(path, frameNumber, 4, !mCurrentJob.jobType().name().startsWith("Max") );
		else
			path.replace(QRegExp("[^\\\\/]*$"), "");
		LOG_5( "ImageView::showFrameCycler: " + path );
		openFrameCycler( path );
	}
}

void JobListWidget::pdPlayer()
{
	if( mCurrentJob.isRecord() ) {
		JobTask cur = mFrameTree->current();
		int frame = cur.isRecord() && cur.job() == mCurrentJob ? cur.frameNumber() : mCurrentJob.minTaskNumber();
		QString imagePath = makeFramePath( mCurrentJob.outputPath(), frame, 4, true /*!mCurrentJob.jobType().name().startsWith("Max")*/ );
		QStringList args;
		args << imagePath;
		args << "--color_space=srgb";
		args << "--alpha=pm";
		IniConfig cfg = userConfig();
		cfg.pushSection( "Assfreezer_Preferences" ); // Should be changed
		QString path;
		path = cfg.readString( "PdPlayer64Path", "C:/Program Files/Pdplayer 64/Pdplayer64.exe" );
		// TODO: Need to use this for quicktimes
		//path = cfg.readString( "PdPlayerPath" );
		cfg.popSection();
		QProcess::startDetached( path, args );
	}
}


void JobListWidget::outputPathExplorer()
{
	JobTask cur = mFrameTree->current();
	if( cur.isRecord() ){
		mImageView->showOutputPath(cur.frameNumber());
	} else {
		Job j = mJobTree->current();
		if( j.isRecord() )
			exploreFile( j.outputPath() );
	}
}

void JobListWidget::exploreJobFile()
{
	if( mCurrentJob.isRecord() ) {
		QString fileName = mCurrentJob.fileName().replace( "/", "\\" ).replace( "N:", "\\\\stryfe\\assburner" );
		exploreFile( fileName );
	}
}

void JobListWidget::changeFrameSelection(int /*frameNumber*/)
{
	/*QTreeWidgetItem * item = mFrameTree->firstChild();
	while(item){
		if( item->isSelected() || (item->text(0).toInt()==frameNumber) )
			FrameListView->setSelected(item, item->text(0).toInt()==frameNumber);
		item = item->nextSibling();
	}*/
}

void JobListWidget::showJobPopup( const QPoint & point )
{
	if( !mJobMenu ) mJobMenu = new FreezerJobMenu(this);
	mJobMenu->popup( mJobTree->mapToGlobal( point ) );
}

void JobListWidget::saveCannedBatchJob()
{
	JobList jobs = mJobTree->selection();
	if( jobs.size() == 1 ) {
		JobBatch jb(jobs[0]);
		BatchSubmitDialog * bsd = new BatchSubmitDialog( this );
		bsd->setSaveCannedBatchMode( true );
		bsd->setName( jb.name() );
		bsd->setCommand( jb.cmd() );
		bsd->exec();
		delete bsd;
	}
}

void JobListWidget::clearJobList()
{
	setJobList(JobList());
}

void JobListWidget::showJobInfo()
{
	bool canEdit = User::hasPerms( "Job", true ) || (mCurrentJob.user() == User::currentUser());
	if( mCurrentJob.getValue( "password" ).isNull() || canEdit )
		restorePopup( RecordPropValTree::showRecords( mCurrentJob, this, canEdit ) );
}

void JobListWidget::showJobStatistics()
{
	JobStatWidget * jsw = new JobStatWidget(0);
	jsw->setJobs( mJobTree->selection() );
	jsw->show();
	restorePopup(jsw);
}

void JobListWidget::setJobPriority()
{
	int total = 0, count = 0;
	JobList jl = mJobTree->selection();
	foreach( Job j, jl ) {
		count++;
		total += j.priority();
	}
	bool ok;
	int priority = QInputDialog::getInteger(this,"Set Job(s) Priority","Set Job(s) Priority", total / count, 1, 99, 1, &ok);
	if( ok ) {
		Database::current()->beginTransaction();
		jl.setPriorities( priority );
		jl.commit();
		Database::current()->commitTransaction();
	}
}

void JobListWidget::showFramePopup(const QPoint & point)
{
	if( !mTaskMenu ) mTaskMenu = new FreezerTaskMenu(this);
	mTaskMenu->popup( mErrorTree->mapToGlobal(point) );
}

void JobListWidget::showErrorPopup(const QPoint & point)
{
	if( !mErrorMenu )
		mErrorMenu = new FreezerErrorMenu( this, mErrorTree->selection(), mErrorTree->model()->rootList() );
	else
		mErrorMenu->setErrors( mErrorTree->selection(), mErrorTree->model()->rootList() );
	mErrorMenu->popup( mErrorTree->mapToGlobal(point) );
}

ImageView * JobListWidget::imageView() const
{
	return mImageView;
}

void JobListWidget::setFrameCacheStatus(int fn, int status)
{
	LOG_5("frameNumber: "+QString::number(fn) + " status:"+QString::number(status));
	QModelIndex idx = ModelIter(mFrameTree->model()).findFirst(0,fn);
	if( idx.isValid() )
		FrameTranslator::data(idx).loadedStatus = status;
}

void JobListWidget::createNewViewFromSelection()
{
	JobList sel = mJobTree->selection();
	if( sel.isEmpty() ) return;
	MainWindow * mw = qobject_cast<MainWindow*>(window());
	if( mw ) {
		JobListWidget * jobListView = new JobListWidget(mw);
		jobListView->setJobList( sel );
		mw->insertView( jobListView );
		mw->setCurrentView( jobListView );
	}
}

void JobListWidget::toggleFilter(bool enable)
{
	mJobTree->enableFilterWidget(enable);
	mFrameTree->enableFilterWidget(enable);
	mErrorTree->enableFilterWidget(enable);
}

void JobListWidget::clearFilters()
{
	mJobTree->mRecordFilterWidget->clearFilters();
	mFrameTree->mRecordFilterWidget->clearFilters();
	mErrorTree->mRecordFilterWidget->clearFilters();
}

void JobListWidget::refreshDepsTab()
{
	JobList sel = mJobTree->selection();
	if( sel.isEmpty() ) return;

#ifdef USE_GRAPHVIZ
	GVGraph * gvg = new GVGraph("test", QApplication::font(), 0.8);

	Index * idx = JobDep::table()->indexFromField( "fkeyJob" );
	idx->cacheIncoming(true);
	JobDepList deps = JobDep::table()->selectFrom( QString("(WITH RECURSIVE job_dep_rec AS ( SELECT jobdep.* from jobdep WHERE fkeyjob IN (%1) OR fkeydep IN (%1) UNION SELECT jd.* FROM job_dep_rec jdr, jobdep jd WHERE jd.fkeyjob=jdr.fkeydep OR jd.fkeydep=jdr.fkeydep) SELECT * FROM job_dep_rec) AS JobDep").arg(sel[0].key()) );
	//JobDepList deps = JobDep::table()->selectFrom( QString("jobdep_recursive('%1') AS JobDep").arg(sel[0].key()) );
	idx->cacheIncoming(false);

	//gvg->setFont(QApplication::font());

	Job j = sel[0];
	QString rootKey = QString::number(j.key());

	gvg->addNode(QString::number(j.key()));
	gvg->setNodeAttr(QString::number(j.key()), "width", "0.8");
	gvg->setNodeAttr(QString::number(j.key()), "height", "0.6");
	gvg->setNodeAttr(QString::number(j.key()), "style", "filled");
	gvg->setNodeAttr(QString::number(j.key()), "fillcolor", "#9BDDFF");
	gvg->setNodeAttr(QString::number(j.key()), "peripheries", "2");

	gvg->setRootNode(QString::number(j.key()));

	foreach( JobDep dep, deps ) {
		QString jobKey = QString::number(dep.job().key());
		QString depKey = QString::number(dep.dep().key());

		gvg->addNode(jobKey);
		if( jobKey!=rootKey ) {
			gvg->setNodeAttr(jobKey, "fillcolor", "#FFFFFF");
			gvg->setNodeAttr(jobKey, "peripheries", "1");
		}
		gvg->addNode(depKey);
		if( depKey!=rootKey ) {
			gvg->setNodeAttr(depKey, "fillcolor", "#FFFFFF");
			gvg->setNodeAttr(depKey, "peripheries", "1");
		}
		gvg->addEdge(depKey, jobKey);
		if( dep.depType() == 2 )
			gvg->setEdgeAttr(QPair<QString, QString>(depKey, jobKey), "style", "dashed" );
	}
	gvg->applyLayout("dot");
	gvg->render("/tmp/gvg.png");
	delete gvg;
	mDepsLabel->setPixmap(QPixmap("/tmp/gvg.png"));
#endif
}


void JobListWidget::populateErrorTreeHeaderMenu( QMenu * menu )
{
	menu->addSeparator();
	QAction * showClearedErrorsAction = menu->addAction( "Show Cleared Errors" );
	showClearedErrorsAction->setCheckable( true );
	showClearedErrorsAction->setChecked( mShowClearedErrors );
	connect( showClearedErrorsAction, SIGNAL( toggled( bool ) ), SLOT( setShowClearedErrors( bool ) ) );
}

void JobListWidget::setShowClearedErrors( bool showClearedErrors )
{
	if( showClearedErrors != mShowClearedErrors ) {
		mShowClearedErrors = showClearedErrors;
		if( mJobTabWidget->currentWidget() == mErrorsTab )
			refreshErrorList();
	}
}

