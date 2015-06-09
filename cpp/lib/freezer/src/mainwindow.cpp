
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of RenderLine.
 *
 * RenderLine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * RenderLine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RenderLine; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <qapplication.h>
#include <qaction.h>
#include <qfiledialog.h>
#include <qinputdialog.h>
#include <qlabel.h>
#include <qmenubar.h>
#include <qmessagebox.h>
#include <qstackedwidget.h>
#include <qstatusbar.h>
#include <qtextstream.h>
#include <qtimer.h>
#include <qtooltip.h>

#include <stdlib.h>

#include "blurqt.h"

#include "assfreezermenus.h"
#include "displayprefsdialog.h"
#include "errorlistwidget.h"
#include "filteredit.h"
#include "graphiteview.h"
#include "hostlistsdialog.h"
#include "hostlistwidget.h"
#include "hostservicematrix.h"
#include "userservicematrix.h"
#include "joblistwidget.h"
#include "projectweightdialog.h"
#include "projectreservedialog.h"
#include "settingsdialog.h"
#include "servicestatusview.h"
#include "svnrev.h"
#include "threadtasks.h"
#include "viewmanager.h"
#include "webview.h"

#include "mainwindow.h"
#include "ui_aboutdialog.h"

void migrateVersion( IniConfig & ini )
{
	ini.pushSection("Assfreezer");
	int iniVersion = ini.readInt("Version");
	ini.popSection();
	
	if( iniVersion < 6294 ) {
		
	}
	if( iniVersion < 6309 ) {
		if( ini.sections().contains( "AFW_Display_Prefs" ) )
			ini.renameSection( "AFW_Display_Prefs", "Assfreezer_Preferences" );
	}
}

MainWindow::MainWindow( QWidget * parent )
: QMainWindow( parent )
, mTabWidget( 0 )
, mStackedWidget( 0 )
, mCurrentView( 0 )
, mJobPage( 0 )
, mHostPage( 0 )
, mCounterLabel( 0 )
, mAdminEnabled( false )
, mCounterActive( false )
, mAutoRefreshTimer( 0 )
{
	FileExitAction = new QAction( "&Quit", this );
	FileExitAction->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_Q ) );
	FileExitAction->setIcon( QIcon( "images/quit.png" ) );

	FileSaveAction = new QAction( "&Save Settings", this );
	FileSaveAction->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_S ) );
	FileSaveAction->setIcon( QIcon( ":/images/saveview" ) );

	HelpAboutAction = new QAction( "About...", this );
	HostServiceMatrixAction = new QAction( "Host Service Matrix...", this );
	HostServiceMatrixAction->setIcon( QIcon( "images/hosts.png" ) );

	UserServiceMatrixAction = new QAction( "User Service Matrix...", this );
	UserServiceMatrixAction->setIcon( QIcon( "images/users.png" ) );

	ProjectWeightingAction = new QAction( "Project Weighting", this );
	ProjectWeightingAction->setIcon( QIcon( ":/images/projectweighting" ) );

	ProjectReserveAction = new QAction( "Project Reserves", this );
	ProjectReserveAction->setIcon( QIcon( ":/images/projectweighting" ) );

	ViewHostsAction = new QAction( "View Hosts", this );
	ViewHostsAction->setCheckable( TRUE );
	ViewHostsAction->setIcon( QIcon( ":/images/view_hosts") );
	ViewJobsAction = new QAction( "View Jobs", this );
	ViewJobsAction->setCheckable( TRUE );
	ViewJobsAction->setChecked(true);
	ViewJobsAction->setIcon( QIcon( ":/images/view_jobs" ) );

	connect( ViewHostsAction, SIGNAL( toggled(bool) ), SLOT( hostViewActionToggled(bool) ) );
	connect( ViewJobsAction, SIGNAL( toggled(bool) ), SLOT( jobViewActionToggled(bool) ) );

	QActionGroup * viewAG = new QActionGroup( this );
	viewAG->addAction( ViewHostsAction );
	viewAG->addAction( ViewJobsAction );

	DisplayPrefsAction = new QAction( "Display Preferences", this );
	DisplayPrefsAction->setIcon( QIcon( ":/images/displaypreferences" ) );
	SettingsAction = new QAction( "Settings", this );
	SettingsAction->setIcon( QIcon( ":/images/settings" ) );

	AdminAction = new QAction( "Admin", this );
	
	AutoRefreshAction = new QAction( "Auto Refresh", this );
	AutoRefreshAction->setIcon( QIcon( "images/auto_refresh.png" ) );
	AutoRefreshAction->setCheckable( true );
	connect( AutoRefreshAction, SIGNAL( toggled( bool ) ), SLOT( setAutoRefreshEnabled( bool ) ) );
	mAutoRefreshTimer = new QTimer(this);
	connect( mAutoRefreshTimer, SIGNAL( timeout() ), SLOT( autoRefresh() ) );

	connect( HostServiceMatrixAction, SIGNAL( triggered(bool) ), SLOT( openHostServiceMatrixWindow() ) );
	connect( UserServiceMatrixAction, SIGNAL( triggered(bool) ), SLOT( openUserServiceMatrixWindow() ) );
	connect( ProjectWeightingAction, SIGNAL( triggered(bool) ), SLOT( showProjectWeightDialog() ) );
	connect( ProjectReserveAction, SIGNAL( triggered(bool) ), SLOT( showProjectReserveDialog() ) );
	connect( HelpAboutAction, SIGNAL( triggered(bool) ), SLOT( showAbout() ) );
	connect( FileExitAction, SIGNAL( triggered(bool) ), qApp, SLOT( closeAllWindows() ) );
	connect( FileSaveAction, SIGNAL( triggered(bool) ), this, SLOT( saveSettings() ) );
	connect( SettingsAction, SIGNAL( triggered(bool) ), SLOT( showSettings() ) );
	connect( DisplayPrefsAction, SIGNAL( triggered(bool) ), SLOT( showDisplayPrefs() ) );
	connect( AdminAction, SIGNAL( triggered(bool) ), SLOT( enableAdmin() ) );

	/* Setup counter */
	mCounterLabel = new QLabel("", statusBar());
	mCounterLabel->setAlignment( Qt::AlignCenter );
	mCounterLabel->setMinimumWidth(350);
	mCounterLabel->setMaximumHeight(20);
	mCounterLabel->installEventFilter(this);
	statusBar()->addPermanentWidget( mCounterLabel );

	mFarmStatusLabel = new QLabel("", statusBar());
	statusBar()->insertPermanentWidget( 0, mFarmStatusLabel );

	/* Set up MainWindow stuff */
	IniConfig & c( config() );
	c.pushSection( "Assfreezer" );
	QString cAppName = c.readString("ApplicationName", "AssFreezer");
	setWindowTitle(cAppName+" - Version " + VERSION);
	//setWindowTitle(cAppName+" - Version " + VERSION + ", build " + SVN_REVSTR);
	setWindowIcon( QIcon(":/images/"+cAppName+"Icon.png" ) );

	Toolbar = new QToolBar( this );
	Toolbar->setIconSize( QSize( 20, 20 ) );
	addToolBar( Toolbar );

	QMenuBar * mb = menuBar();
	mFileMenu = mb->addMenu( "&File" );
	mFileMenu->addAction( FileSaveAction );
	mFileMenu->addAction( FileExitAction );

	mToolsMenu = mb->addMenu( "&Tools" );
	mToolsMenu->setObjectName( "Freezer_Tools_Menu" );
	connect( mToolsMenu, SIGNAL( aboutToShow() ), SLOT( populateToolsMenu() ) );

	mOptionsMenu = mb->addMenu( "&Options" );
	connect( mOptionsMenu, SIGNAL( aboutToShow() ), SLOT( populateViewMenu() ) );

	mViewMenu = mb->addMenu( "&View" );
//	connect( mViewMenu, SIGNAL( aboutToShow() ), SLOT( populateWindowMenu() ) );
	mViewMenu->addAction( ViewJobsAction );
	mViewMenu->addAction( ViewHostsAction );
	mViewMenu->addSeparator();
	mNewJobViewAction = mViewMenu->addAction( "New &Job View" );
	mNewJobViewAction->setIcon( QIcon( ":/images/newview" ) );
	mNewJobViewAction->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_J ) );
	mNewHostViewAction = mViewMenu->addAction( "New &Host View" );
	mNewHostViewAction->setIcon( QIcon( ":/images/newview" ) );
	mNewHostViewAction->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_H ) );
	mNewErrorViewAction = mViewMenu->addAction( "New &Error View" );
	mNewErrorViewAction->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_E ) );
	mNewGraphiteViewAction = mViewMenu->addAction( "New Graphite View" );
	mNewWebViewAction = mViewMenu->addAction( "New Web View" );
	mRestoreViewMenu = mViewMenu->addMenu( "Rest&ore View" );

	connect( mRestoreViewMenu, SIGNAL( aboutToShow() ), SLOT( populateRestoreViewMenu() ) );
	connect( mRestoreViewMenu, SIGNAL( triggered( QAction * ) ), SLOT( restoreViewActionTriggered( QAction * ) ) );

	mViewMenu->addSeparator();
	mCloneViewAction = mViewMenu->addAction( "Clon&e Current View" );
	mCloneViewAction->setIcon( QIcon( ":/images/copy" ) );
	mSaveViewAsAction = mViewMenu->addAction( "&Save Current View As..." );
	mCloseViewAction = mViewMenu->addAction( "&Close Current View" );
	mCloseViewAction->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_W ) );
	QAction * renameViewAction = mViewMenu->addAction( "Re&name Current View" );
	mMoveViewLeftAction = mViewMenu->addAction( "Move Current View &Left" );
	mMoveViewLeftAction->setShortcut( QKeySequence( Qt::CTRL + Qt::SHIFT + Qt::Key_Left ) );
	mMoveViewRightAction = mViewMenu->addAction( "Move Current View &Right" );
	mMoveViewRightAction->setShortcut( QKeySequence( Qt::CTRL + Qt::SHIFT + Qt::Key_Right ) );

	QAction * mSaveViewToFileAction = mViewMenu->addAction( "Save View To &File" );
	mSaveViewToFileAction->setIcon( QIcon( ":/images/saveview" ) );
	mSaveViewToFileAction->setShortcut( QKeySequence( Qt::CTRL + Qt::SHIFT + Qt::Key_S ) );

	QAction * mLoadViewFromFileAction = mViewMenu->addAction( "Load View fr&om File" );
	mLoadViewFromFileAction->setIcon( QIcon( ":/images/loadview" ) );
	mLoadViewFromFileAction->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_O ) );

	mViewMenu->addSeparator();

	connect( mNewJobViewAction, SIGNAL( triggered(bool) ), SLOT( createJobView() ) );
	connect( mNewHostViewAction, SIGNAL( triggered(bool) ), SLOT( createHostView() ) );
	connect( mNewGraphiteViewAction, SIGNAL( triggered(bool) ), SLOT( createGraphiteView() ) );
	connect( mNewWebViewAction, SIGNAL( triggered(bool) ), SLOT( createWebView() ) );
	connect( mNewErrorViewAction, SIGNAL( triggered(bool) ), SLOT( createErrorView() ) );
	connect( mCloneViewAction, SIGNAL( triggered(bool) ), SLOT( cloneCurrentView() ) );
	connect( mSaveViewAsAction, SIGNAL( triggered(bool) ), SLOT( saveCurrentViewAs() ) );
	connect( mCloseViewAction, SIGNAL( triggered(bool) ), SLOT( closeCurrentView() ) );
	connect( mMoveViewLeftAction, SIGNAL( triggered(bool) ), SLOT( moveCurrentViewLeft() ) );
	connect( mMoveViewRightAction, SIGNAL( triggered(bool) ), SLOT( moveCurrentViewRight() ) );
	connect( renameViewAction, SIGNAL( triggered(bool) ), SLOT( renameCurrentView() ) );

	connect( mSaveViewToFileAction, SIGNAL( triggered(bool) ), SLOT( saveCurrentViewToFile() ) );
	connect( mLoadViewFromFileAction, SIGNAL( triggered(bool) ), SLOT( loadViewFromFile(bool) ) );

	mHelpMenu = mb->addMenu( "&Help" );
	mHelpMenu->addAction( HelpAboutAction );

	IniConfig & ini = ::userConfig();
	migrateVersion(ini);

	QPalette p = palette();
	QColor fg(155,207,226), bg(8,5,76);

	ini.pushSection("Assfreezer_Preferences");

	options.mHostColors = new ViewColors("HostView");
	options.mHostColors->mColors
		<< ColorOption( "default", fg, bg )
		<< ColorOption( "ready" )
		<< ColorOption( "assigned", QColor(47,191,255) )
		<< ColorOption( "busy", QColor(47,191,255) )
		<< ColorOption( "copy", QColor(47,191,255) )
		<< ColorOption( "restart" )
		<< ColorOption( "client-update" )
		<< ColorOption( "offline", QColor(125,122,156) )
		<< ColorOption( "maintenance", QColor(225,100,100) );

	options.mHostColors->readColors();

	options.mJobColors = new ViewColors("JobView");
	options.mJobColors->mColors
		<< ColorOption( "default", fg, bg )
		<< ColorOption( "submit", QColor(51,49,91) )
		<< ColorOption( "verify", QColor(51,49,91) )
		<< ColorOption( "ready", fg )
		<< ColorOption( "started", QColor(47,191,255) )
		<< ColorOption( "suspended", QColor(164,141,199) )
		<< ColorOption( "holding", QColor(164,141,199) )
		<< ColorOption( "done", QColor(125,122,156) )
		<< ColorOption( "group", fg, bg );
	
	options.mFrameColors = new ViewColors("FrameView");
	options.mFrameColors->mColors
		<< ColorOption( "default", fg, bg )
		<< ColorOption( "new" )
		<< ColorOption( "assigned", fg )
		<< ColorOption( "busy", QColor(47,191,255) )
		<< ColorOption( "done", QColor(125,122,156) );
	
	options.mErrorColors = new ViewColors("ErrorView");
	options.mErrorColors->mColors
		<< ColorOption( "default", fg, bg );

	options.mJobColors->readColors();
	options.mFrameColors->readColors();
	options.mErrorColors->readColors();

	options.appFont = ini.readFont( "AppFont" );
	options.jobFont = ini.readFont( "JobFont" );
	options.frameFont = ini.readFont( "FrameFont" );
	options.summaryFont = ini.readFont( "SummaryFont" );
	options.frameCyclerPath = ini.readString( "FrameCyclerPath" );
	options.frameCyclerArgs = ini.readString( "FrameCyclerArgs" );

	options.mRefreshInterval = ini.readInt( "AutoRefreshIntervalMinutes", 5 );
	options.mAutoRefreshOnWindowActivation = ini.readBool( "AutoRefreshOnWindowActivation", true );
	options.mRefreshOnViewChange = ini.readBool( "RefreshOnViewChange", options.mAutoRefreshOnWindowActivation );
	setAutoRefreshEnabled( ini.readBool( "AutoRefreshEnabled", false ) );
	options.mCounterRefreshInterval = ini.readInt( "CounterRefreshIntervalSeconds", 30 );
	options.mLimit = ini.readInt( "QueryLimit", 1000 );
	options.mDaysLimit = ini.readInt( "DaysLimit", 3 );
	qApp->setStartDragDistance( ini.readInt( "DragStartDistance", 4 ) );
	ini.popSection();

	applyOptions();

	restoreViews();

	setCounterState(true);
}

MainWindow::~MainWindow()
{
	FreezerCore::instance()->cancelObjectTasks(this);
}

void MainWindow::closeEvent( QCloseEvent * ce )
{
	saveSettings();
	QMainWindow::closeEvent(ce);
}

void MainWindow::saveSettings()
{
	IniConfig & ini = userConfig();
	ini.pushSection( "MainWindow" );
	ini.writeString( "FrameGeometry", QString("%1,%2,%3,%4").arg( pos().x() ).arg( pos().y() ).arg( size().width() ).arg( size().height() ) );
	ini.writeString( "WindowState", isMaximized() ? "Maximized" : "Normal" );
	ini.popSection();

	options.mJobColors->writeColors();
	options.mHostColors->writeColors();
	options.mFrameColors->writeColors();
	options.mErrorColors->writeColors();

	ini.pushSection("Assfreezer_Preferences" );
	ini.writeFont( "AppFont", options.appFont );
	ini.writeFont( "JobFont", options.jobFont );
	ini.writeFont( "FrameFont", options.frameFont );
	ini.writeFont( "SummaryFont", options.summaryFont );
	ini.writeString( "FrameCyclerPath", options.frameCyclerPath );
	ini.writeString( "FrameCyclerArgs", options.frameCyclerArgs );
	ini.writeBool( "AutoRefreshEnabled", AutoRefreshAction->isChecked() );
	ini.writeInt( "AutoRefreshIntervalMinutes", options.mRefreshInterval );
	ini.writeBool( "AutoRefreshOnWindowActivation", options.mAutoRefreshOnWindowActivation );
	ini.writeBool( "RefreshOnViewChange", options.mRefreshOnViewChange );
	ini.writeInt( "CounterRefreshIntervalSeconds", options.mCounterRefreshInterval );
	ini.writeInt( "QueryLimit", options.mLimit );
	ini.writeInt( "DaysLimit", options.mDaysLimit );
	ini.writeInt( "DragStartDistance", qApp->startDragDistance() );
	ini.popSection();

	ini.pushSection("Assfreezer");
	ini.writeString("Version",SVN_REVSTR);
	ini.popSection();
	
	saveViews();
	statusBar()->showMessage("Settings saved to "+ini.fileName());
}

bool MainWindow::event( QEvent * event )
{
	if( event->type() == QEvent::WindowActivate && autoRefreshOnWindowActivation() )
		autoRefresh();

	switch( (int)event->type() ) {
		case COUNTER:
		{
			CounterTask * ct = (CounterTask*)event;
			CounterState & cs = ct->mReturn;
			QString counterText;
			QTextStream ts( &counterText, QIODevice::WriteOnly );
			ts << "<b>Hosts</b>( " << cs.hostsTotal << ", " << cs.hostsActive <<", "<< cs.hostsReady;
			ts << " )   <b>Jobs</b>( "<< cs.jobsTotal << ", " << cs.jobsActive << ", " << cs.jobsDone << " )";
			mCounterLabel->setText( counterText );
			mCounterLabel->setToolTip( "Hosts( Total, Active, Ready )  Jobs( Total, Active, Done )"  );
			updateFarmStatus( ct->mManagerService );
			QTimer::singleShot( options.mCounterRefreshInterval * 1000, this, SLOT( updateCounter() ) );
			break;
		}
		default:
			break;
	}
	return QMainWindow::event(event);
}

void MainWindow::keyPressEvent( QKeyEvent * event )
{
	if( event->key() == Qt::Key_F5 ) {
		if( mCurrentView )
			mCurrentView->refresh();
		event->accept();
	} else
		event->ignore();
}

void MainWindow::saveViews()
{
	IniConfig & ini = userConfig();
	
	// Remove the entire section to remove old entries that are above our current view count
	ini.removeSection( "Assfreezer_Open_Views" );

	ini.pushSection("Assfreezer_Open_Views");
	ini.writeInt("ViewCount",mViews.size());
	if( mCurrentView ) 
		ini.writeString("CurrentView", mCurrentView->viewCode());
	LOG_5( "Saving " + QString::number( mViews.size() ) + " views" );
	int i = 0;
	foreach( FreezerView * view, mViews ) {
		++i;
		LOG_5( "Saving View " + view->viewName() + " Code: " + view->viewCode() );
		ini.writeString( QString("ViewCode%1").arg(i), view->viewCode() );
		ini.pushSection("View_" + view->viewCode());
		view->save(ini);
		ini.popSection();
	}
	ini.popSection();

	ini.removeSection("Assfreezer_Saved_Views");
	ini.pushSection("Assfreezer_Saved_Views");
	ViewManager::instance()->writeSavedViews(ini);
	ini.popSection();

	foreach( QString section, ini.sections() ) {
		if( !section.startsWith( "View_" ) ) continue;
		
		QString code = section.mid(5);
		if( code.contains( ":" ) )
			code = code.left( code.indexOf(":") );
		
		if( ViewManager::instance()->hasSavedView(code) )
			continue;
		
		bool isOpenView = false;
		foreach( FreezerView * view, mViews )
			if( view->viewCode() == code ) {
				isOpenView = true;
				break;
			}
		if( isOpenView ) continue;

		// Not an open or saved view, so remove it
		ini.removeSection( section );
	}
}

void MainWindow::saveView( FreezerView * view )
{
	LOG_5( "Saving View " + view->viewName() );
	IniConfig & ini = userConfig();
	ini.pushSection("View_" + view->viewCode());
	view->save(ini);
	ini.popSection();
}

void MainWindow::restoreViews()
{
	IniConfig & ini = userConfig();

	ini.pushSection("Assfreezer");
	bool upgradeMode = ini.readInt( "Version" ) < 10545;
	ini.popSection();
	
	ini.pushSection("Assfreezer_Saved_Views");
	ViewManager::instance()->readSavedViews(ini);
	ini.popSection();

	FreezerView * currentView = 0;
	ini.pushSection("Assfreezer_Open_Views");
	if( upgradeMode ) {
		QString currentViewName = ini.readString("CurrentView");
		int viewCount = ini.readInt("ViewCount");
		for( int i = 1; i <= viewCount; ++i ) {
			QString viewName = ini.readString(QString("ViewName%1").arg(i));
			if( viewName.isEmpty() ) continue;
			QString newViewCode = FreezerView::generateViewCode();
			foreach( QString section, ini.sections() ) {
				QString subSection = "View_" + viewName;
				if( section.startsWith( subSection + ":") || section == subSection )
					ini.copySection( section, "View_" + newViewCode + section.mid(subSection.size()) );
			}
			FreezerView * view = restoreSavedView( newViewCode, false );
			if( !currentView || viewName == currentViewName )
				currentView = view;
		}
	} else {
		QString currentViewCode = ini.readString("CurrentView");
		int viewCount = ini.readInt("ViewCount");
		for( int i = 1; i <= viewCount; ++i ) {
			QString viewCode = ini.readString(QString("ViewCode%1").arg(i));
			if( viewCode.isEmpty() ) continue;
			FreezerView * view = restoreSavedView( viewCode, false );
			if( !currentView || viewCode == currentViewCode )
				currentView = view;
		}
	}
	ini.popSection();

	// testing
	if( mViews.size() == 0 ) {
		insertView(new JobListWidget(this));
		insertView(new HostListWidget(this));
	}
	
	if( currentView )
		setCurrentView(currentView);
	checkViewModeChange();
}

FreezerView * MainWindow::restoreSavedView( const QString & viewCode, bool updateWindow )
{
	IniConfig & ini = userConfig();
	ini.pushSection("View_" + viewCode);
	FreezerView * view = restoreView( ini, viewCode, updateWindow );
	ini.popSection();
	return view;
}

FreezerView * MainWindow::restoreView( IniConfig & viewDesc, const QString & viewCode, bool updateWindow, bool forceFullRestore )
{
	QString type = viewDesc.readString( "ViewType" );
	QString name = viewDesc.readString( "ViewName" );
	LOG_5( "Restoring View " + name + " code: " + viewCode + " type: " + type );
	FreezerView * view = 0;
	if( type == "ErrorList" )
		view = new ErrorListWidget(this);
	else if( type == "HostList" )
		view = new HostListWidget(this);
	else if( type == "JobList" )
		view = new JobListWidget(this);
	else if( type == "GraphiteView" )
		view = new GraphiteView(this);
	else if( type == "WebView" )
		view = new WebView(this);
	if( view ) {
		view->setViewName( name );
		view->setViewCode( viewCode );
		view->restore(viewDesc,forceFullRestore);
		insertView(view,updateWindow);
	} else
		LOG_1( "Unable to create view of type: " + type );
	return view;
}

void MainWindow::cloneCurrentView()
{
	if( mCurrentView )
		cloneView(mCurrentView);
}

void MainWindow::cloneView( FreezerView * view )
{
	IniConfig empty;
	view->save(empty,true);
	view = restoreView(empty, view->viewCode(), true, true);
	// Give it a unique code
	view->setViewCode( FreezerView::generateViewCode() );
}

void MainWindow::insertView( FreezerView * view, bool checkViewModeCheckCurrent )
{
	mViews += view;

	// Keep pointers to the first job page and first host page
	if( !mJobPage && view->inherits( "JobListWidget" ) )
		mJobPage = qobject_cast<JobListWidget*>(view);

	if( !mHostPage && view->inherits( "HostListWidget" ) )
		mHostPage = qobject_cast<HostListWidget*>(view);

	if( checkViewModeCheckCurrent ) {
		if( !checkViewModeChange() ) {
			if( mStackedWidget ) {
				mStackedWidget->addWidget(view);
			} else if( mTabWidget ) {
				LOG_3( "Adding tab for view: " + view->viewName() );
				mTabWidget->addTab( view, view->viewName() );
			}
		}
		if( !mCurrentView ) setCurrentView( view );
	}
}

bool MainWindow::checkViewModeChange()
{
	bool needTabs = mViews.size() > 2 || (mViews.size() == 2 && (!mHostPage || !mJobPage));
	if( needTabs && !mTabWidget )
		setupTabbedView();
	else if( !needTabs && !mStackedWidget && mViews.size() )
		setupStackedView();
	else
		return false;
	repopulateToolBar();
	return true;
}

void MainWindow::removeView( FreezerView * view )
{
	mViews.removeAll( view );
	if( view == mCurrentView )
		showLastVisitedView();
	if( mStackedWidget )
		mStackedWidget->removeWidget(view);
	if( mTabWidget )
		mTabWidget->removeTab(mTabWidget->indexOf(view));
	if( view == mJobPage || view == mHostPage ) {
		if( mJobPage == view ) mJobPage = 0;
		if( mHostPage == view ) mHostPage = 0;
		foreach( FreezerView * view, mViews ) {
			if( !mJobPage && view->inherits( "JobListWidget" ) ) mJobPage = qobject_cast<JobListWidget*>(view);
			if( !mHostPage && view->inherits( "HostListWidget" ) ) mHostPage = qobject_cast<HostListWidget*>(view);
		}
	}
	delete view;
	QTimer::singleShot( 0, this, SLOT( checkViewModeChange() ) );
}

void MainWindow::currentTabChanged( int index )
{
	setCurrentView( qobject_cast<FreezerView*>(mTabWidget->widget(index)) );
}

void MainWindow::setCurrentView( const QString & viewName )
{
	if( mTabWidget ) {
		for( int i = 0; i<mTabWidget->count(); i++ ) {
			FreezerView * view = qobject_cast<FreezerView*>(mTabWidget->widget(i));
			if( view->viewName() == viewName )
				setCurrentView( view );
		}
	}
}

void MainWindow::setCurrentView( FreezerView * view )
{
	if( view == mCurrentView ) return;
	
	setUpdatesEnabled( false );
	if( mCurrentView ) {
		disconnect( mCurrentView, SIGNAL( statusBarMessageChanged( const QString & ) ), statusBar(), SLOT( showMessage( const QString & ) ) );
		QToolBar * tb = mCurrentView->toolBar(this);
		if( tb ) tb->hide();
		mViewVisitList.removeAll(mCurrentView);
		mViewVisitList.push_back(mCurrentView);
	}

	mCurrentView = view;
	if( mCurrentView ) {
		if( mStackedWidget && mCurrentView != mStackedWidget->currentWidget() )
			mStackedWidget->setCurrentWidget( mCurrentView );
		else if( mTabWidget && mCurrentView != mTabWidget->currentWidget() )
			mTabWidget->setCurrentWidget( mCurrentView );
		QString currentMessage = view->statusBarMessage();
		LOG_5( "currentMessage: " + currentMessage );
		if( currentMessage.isEmpty() )
			statusBar()->clearMessage();
		else
			statusBar()->showMessage(currentMessage);
		connect( view, SIGNAL( statusBarMessageChanged( const QString & ) ), statusBar(), SLOT( showMessage( const QString & ) ) );
		QToolBar * tb = mCurrentView->toolBar(this);
		if( tb ) {
			addToolBar(tb);
			tb->show();
		}
		bool isHostView = view->inherits("HostListWidget");
		if( mStackedWidget ) {
			ViewJobsAction->setChecked(!isHostView);
			ViewHostsAction->setChecked(isHostView);
		}

		emit currentViewChanged( mCurrentView );
		
		// Always refresh the first time a view is shown.
		if( options.mRefreshOnViewChange || mCurrentView->refreshCount() == 0 )
			mCurrentView->refresh();
	} else
		statusBar()->clearMessage();

	mMoveViewLeftAction->setEnabled( mTabWidget && (mTabWidget->currentIndex() > 0) );
	mMoveViewRightAction->setEnabled( mTabWidget && (mTabWidget->currentIndex() < mTabWidget->count() - 1) );

	setUpdatesEnabled( true );
}

void MainWindow::showNextView()
{
	if( mTabWidget && mTabWidget->count() > 1 ) {
		int index = (mTabWidget->currentIndex() + 1) % mTabWidget->count();
		setCurrentView( qobject_cast<FreezerView*>(mTabWidget->widget(index)) );
	}
}

void MainWindow::showLastVisitedView()
{
	FreezerView * view = 0;
	while( !view && mViewVisitList.size() ) {
		view = mViewVisitList.back();
		mViewVisitList.pop_back();
		// Check if view has been deleted
		if( !mViews.contains( view ) )
			view = 0;
	}
	if( !view && mViews.size() ) {
		view = mViews.front();
		if( view == mCurrentView && mViews.size() > 1 )
			view = mViews[1];
	}
	setCurrentView(view);
}

void MainWindow::setupStackedView()
{
	mStackedWidget = new QStackedWidget(this);
	disconnect( mTabWidget, SIGNAL( currentChanged(int) ), this, 0 );
	foreach( FreezerView * view, mViews ) {
		if( mTabWidget ) mTabWidget->removeTab(mTabWidget->indexOf(view));
		mStackedWidget->addWidget(view);
	}
	setCentralWidget(mStackedWidget);
	if( mCurrentView )
		mStackedWidget->setCurrentWidget( mCurrentView );
	mTabWidget = 0;
}

void MainWindow::setupTabbedView()
{
	mTabWidget = new QTabWidget(this);
	mTabWidget->installEventFilter(this);
	foreach( FreezerView * view, mViews ) {
		if( mStackedWidget )
			mStackedWidget->removeWidget(view);
		mTabWidget->addTab( view, view->viewName() );
	}
	if( mCurrentView )
		mTabWidget->setCurrentWidget( mCurrentView );
	connect( mTabWidget, SIGNAL( currentChanged(int) ), SLOT( currentTabChanged(int) ) );
	setCentralWidget( mTabWidget );
	mStackedWidget = 0;
}

void MainWindow::repopulateToolBar()
{
	Toolbar->clear();
	if( mStackedWidget ) {
		Toolbar->addAction( ViewHostsAction );
		Toolbar->addAction( ViewJobsAction );
		Toolbar->addSeparator();
		if( mCurrentView ) {
			bool isHostView = mCurrentView->inherits("HostListWidget");
			ViewJobsAction->setChecked(!isHostView);
			ViewHostsAction->setChecked(isHostView);
		}
	}
	Toolbar->addAction( AutoRefreshAction );
}

void MainWindow::createJobView()
{
	FreezerView * view = new JobListWidget(this);
	renameView(view);
	insertView(view);
}

void MainWindow::createHostView()
{
	FreezerView * view = new HostListWidget(this);
	renameView(view);
	insertView(view);
}

void MainWindow::createGraphiteView()
{
	FreezerView * view = new GraphiteView(this);
	renameView(view);
	insertView(view);
}

void MainWindow::createWebView()
{
	insertView( new WebView(this) );
}

void MainWindow::createErrorView()
{
	insertView( new ErrorListWidget(this) );
}

void MainWindow::closeCurrentView()
{
	if( mCurrentView )
		removeView( mCurrentView );
}

void MainWindow::saveViewToFile( FreezerView * view )
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save View to File"),
													"",
													tr("ViewConfig (*.ini)"));
	IniConfig newConfig = IniConfig(fileName);
	newConfig.pushSection("View_SavedToFile");
	view->save(newConfig);
	newConfig.popSection();
	newConfig.writeToFile();
}

void MainWindow::saveCurrentViewToFile()
{
	if( mCurrentView )
		saveViewToFile( mCurrentView );
}

void MainWindow::loadViewFromFile(bool)
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load View From"),
													"",
													tr("ViewConfig (*.ini)"));
	IniConfig newConfig(fileName);
	newConfig.pushSection("View_SavedToFile");
	QString viewName = newConfig.readString( "ViewName" );
	restoreView(newConfig, viewName);
}

void MainWindow::loadViewFromFile(const QString & fileName)
{
	IniConfig newConfig(fileName);
	newConfig.pushSection("View_SavedToFile");
	QString viewName = newConfig.readString( "ViewName" );
	restoreView(newConfig, viewName);
}

void MainWindow::renameCurrentView()
{
	if( mCurrentView ) renameView( mCurrentView );
}

// Pops up dialog to prompt user
void MainWindow::renameView( FreezerView * view )
{
	while( 1 ) {
		bool okay;
		QString newViewName = QInputDialog::getText( this, "Rename View", "Enter new view name", QLineEdit::Normal, view->viewName(), &okay );
		if( okay ) {
			if( newViewName.isEmpty() ) {
				QMessageBox::warning( this, "Invalid View Name", "View name cannot be empty" );
				continue;
			}

			view->setViewName( newViewName );

			if( mTabWidget ) {
				int idx = mTabWidget->indexOf(view);
				mTabWidget->setTabText(idx,newViewName);
			}
		}
		break;
	}
}

void MainWindow::saveCurrentViewAs()
{
	saveViewAs( mCurrentView );
}

void MainWindow::saveViewAs( FreezerView * view )
{
	while( 1 ) {
		bool okay;
		QString newViewName = QInputDialog::getText( this, "Save View As...", "Enter name to save view as", QLineEdit::Normal, view->viewName(), &okay );
		if( okay ) {
			if( newViewName.isEmpty() ) {
				QMessageBox::warning( this, "Invalid View Name", "View name cannot be empty" );
				continue;
			}

			if( ViewManager::instance()->savedViewNames().contains( newViewName ) ) {
				QMessageBox::StandardButton result = QMessageBox::question( this, "Overwrite '" + newViewName + "'?", "There is already a saved view named '" + newViewName + "'.  Do you wish to overwrite it?", QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel );
				if( result == QMessageBox::Cancel )
					return;

				if( result == QMessageBox::No )
					continue;
				
				ViewManager::instance()->removeSavedView( newViewName );
			}
			view->setViewName( newViewName );

			if( mTabWidget ) {
				int idx = mTabWidget->indexOf(view);
				mTabWidget->setTabText(idx,newViewName);
			}
		
			saveView( view );
			ViewManager::instance()->addSavedView( newViewName, view->viewCode() );
		}
		break;
	}
	
}

void MainWindow::moveViewLeft( FreezerView * view )
{
	if( view ) {
		int idx = mViews.indexOf( view );
		if( idx > 0 ) {
			mViews.removeAt(idx);
			mViews.insert(idx-1,view);
			if( mTabWidget ) {
				bool sb = mTabWidget->signalsBlocked();
				mTabWidget->blockSignals(true);
				mTabWidget->removeTab(idx);
				mTabWidget->insertTab(idx-1,view,view->viewName());
				if( view == mCurrentView )
					mTabWidget->setCurrentWidget(view);
				mTabWidget->blockSignals(sb);
			}
		}
	}

}

void MainWindow::moveViewRight( FreezerView * view )
{
	if( view ) {
		int idx = mViews.indexOf( view );
		if( idx < mViews.size() - 1 ) {
			mViews.removeAt(idx);
			mViews.insert(idx+1,view);
			if( mTabWidget ) {
				bool sb = mTabWidget->signalsBlocked();
				mTabWidget->blockSignals(true);
				mTabWidget->removeTab(idx);
				mTabWidget->insertTab(idx+1,view,view->viewName());
				if( view == mCurrentView )
					mTabWidget->setCurrentWidget(view);
				mTabWidget->blockSignals(sb);
			}
		}
	}
}

void MainWindow::moveCurrentViewLeft()
{
	moveViewLeft( mCurrentView );
}

void MainWindow::moveCurrentViewRight()
{
	moveViewRight( mCurrentView );
}

void MainWindow::populateToolsMenu()
{
	if( User::hasPerms( "HostService", true ) )
		mToolsMenu->addAction( HostServiceMatrixAction );
	if( User::hasPerms( "UserService", true ) )
		mToolsMenu->addAction( UserServiceMatrixAction );

	mToolsMenu->addAction( ProjectWeightingAction );
	mToolsMenu->addAction( ProjectReserveAction );
	mToolsMenu->addAction( "Manage My Host Lists...", this, SLOT( showManageHostListsDialog() ) );
	
	FreezerMenuFactory::instance()->aboutToShow(mToolsMenu);
	// We only need to populate this menu once
	mToolsMenu->disconnect( this, SLOT( populateToolsMenu() ) );
}

void MainWindow::populateViewMenu()
{
	mOptionsMenu->clear();

	mOptionsMenu->addAction( AutoRefreshAction );

	if( mCurrentView )
		mCurrentView->populateViewMenu( mOptionsMenu );

	mOptionsMenu->addSeparator();

	mOptionsMenu->addAction( DisplayPrefsAction );
	mOptionsMenu->addAction( SettingsAction );
}

void MainWindow::populateRestoreViewMenu()
{
	mRestoreViewMenu->clear();
	QStringList views = ViewManager::instance()->savedViewNames();
	views.sort();
	foreach( QString viewName, views )
		mRestoreViewMenu->addAction( viewName );	
}

void MainWindow::restoreViewActionTriggered(QAction * action)
{
	restoreSavedView( action->text() );
}

void MainWindow::hostViewActionToggled(bool en)
{
	if( en ) showHostView();
}

void MainWindow::jobViewActionToggled(bool en)
{
	if( en ) showJobView();
}

void MainWindow::showHostView()
{
	if( !mHostPage )
		insertView( new HostListWidget(this) );
	setCurrentView( mHostPage );
	ViewHostsAction->setChecked(true);
}

void MainWindow::showJobView()
{
	if( !mJobPage )
		insertView( new JobListWidget(this) );
	setCurrentView( mJobPage );
	ViewJobsAction->setChecked(true);
}

void MainWindow::showProjectWeightDialog()
{
	(new ProjectWeightDialog(this))->show();
}

void MainWindow::showProjectReserveDialog()
{
	(new ProjectReserveDialog(this))->show();
}

void MainWindow::showManageHostListsDialog()
{
	HostListsDialog * hld = new HostListsDialog( this );
	hld->setAttribute( Qt::WA_DeleteOnClose );
	hld->show();
}

bool MainWindow::autoRefreshEnabled() const
{
	return AutoRefreshAction->isChecked();
}

bool MainWindow::autoRefreshOnWindowActivation() const
{
	return options.mAutoRefreshOnWindowActivation;
}

void MainWindow::setAutoRefreshEnabled( bool enabled )
{
	if( AutoRefreshAction->isChecked() != enabled ) AutoRefreshAction->setChecked( enabled );

	if( enabled )
		mAutoRefreshTimer->start( options.mRefreshInterval * 60 * 1000 );
	else
		mAutoRefreshTimer->stop();
}

void MainWindow::setAutoRefreshOnWindowActivation( bool enabled )
{
	options.mAutoRefreshOnWindowActivation = enabled;
}

void MainWindow::autoRefresh()
{
	if( mCurrentView ) mCurrentView->refresh();
	// Restart the timer, since this could have been called
	// from mainwindow activation or elsewhere.
	setAutoRefreshEnabled( AutoRefreshAction->isChecked() );
}

void MainWindow::openHostServiceMatrixWindow()
{
	(new HostServiceMatrixWindow(this))->show();
}

void MainWindow::openUserServiceMatrixWindow()
{
	(new UserServiceMatrixWindow(this))->show();
}

// Turns the update counter on or off
void MainWindow::setCounterState( bool cs )
{
	if( cs != mCounterActive ) {
		mCounterActive = cs;
		if( cs ) updateCounter();
	}
}

void MainWindow::showDisplayPrefs()
{
	DisplayPrefsDialog opts( this );
	connect( &opts, SIGNAL( apply() ), SLOT( applyOptions() ) );
	if( opts.exec() == QDialog::Accepted )
		applyOptions();
}

void MainWindow::showSettings()
{
	SettingsDialog opts( this );
	connect( &opts, SIGNAL( apply() ), SLOT( applyOptions() ) );
	if( opts.exec() == QDialog::Accepted )
		applyOptions();
}

void MainWindow::applyOptions()
{
	bool ue = updatesEnabled();
	setUpdatesEnabled(false);
	QApplication * app = (QApplication*)QApplication::instance();
	app->setFont( options.appFont );
	foreach( FreezerView * view, mViews )
		view->applyOptions();

	// Reset the interval
	if( AutoRefreshAction->isChecked() )
		setAutoRefreshEnabled( true );

	setUpdatesEnabled(ue);
}

void MainWindow::enableAdmin()
{
	mAdminEnabled=true;
}

void MainWindow::showAbout()
{
	QDialog dialog( this );
	Ui::AboutDialog ui;
	ui.setupUi( &dialog );
	dialog.exec();
}

void MainWindow::updateCounter()
{
	if( mCounterActive ) {
		FreezerCore::addTask( new CounterTask( this ) );
		FreezerCore::wakeup();
	}
}

void MainWindow::showTabMenu( const QPoint & pos, FreezerView * view )
{
	QMenu * menu = new QMenu(this);
	QAction * close = menu->addAction( "&Close View" );
	close->setIcon( QIcon( ":/images/close" ) );
	QAction * moveLeft = menu->addAction( "Move View &Left" );
	moveLeft->setIcon( QIcon( ":/images/moveleft" ) );
	QAction * moveRight = menu->addAction( "Move View &Right" );
	moveRight->setIcon( QIcon( ":/images/moveright" ) );
	QAction * rename = menu->addAction( "Re&name View" );
	rename->setIcon( QIcon( ":/images/rename" ) );
	QAction * clone = menu->addAction( "Clon&e View" );
	QAction * result = menu->exec(pos);
	QAction * save = menu->addAction( "&Save View" );

	if( result == close ) {
		removeView(view);
	} else if( result == moveLeft ) {
		moveViewLeft( view );
	} else if( result == moveRight ) {
		moveViewRight( view );
	} else if( result == rename ) {
		renameView( view );
	} else if( result == save ) {
		saveViewToFile(view);
	} else if( result == clone ) {
		cloneView(view);
	}
}

class TabBarHack : public QTabWidget { public: QTabBar * tb() { return tabBar(); } };

bool MainWindow::eventFilter( QObject * o, QEvent * event )
{
	QTabBar * tb = mTabWidget ? ((TabBarHack*)mTabWidget)->tb() : 0;
	if( tb && o == mTabWidget && event->type() == QEvent::MouseButtonPress ) {
		QMouseEvent * mouseEvent = (QMouseEvent*)event;
		if( mouseEvent->button() == Qt::RightButton ) {
			for( int i = tb->count()-1; i >= 0; --i ) {
				QPoint tabBarPos = tb->mapFromParent(mouseEvent->pos());
				if( tb->tabRect(i).contains( tabBarPos ) ) {
					showTabMenu( mouseEvent->globalPos(), qobject_cast<FreezerView*>(mTabWidget->widget(i)) );
					break;
				}
			}
		}
	}
	if( o == mCounterLabel && event->type() == QEvent::MouseButtonPress ) {
		QMouseEvent * mouseEvent = (QMouseEvent*)event;
		if( mouseEvent->button() == Qt::LeftButton ) {
			ServiceStatusView * serviceStatusView = new ServiceStatusView(this);
			serviceStatusView->setAttribute( Qt::WA_DeleteOnClose );
			QSize sh = serviceStatusView->allContentsSizeHint();
			QPoint bottomRight( mapToGlobal(QPoint(width(),0)).x(), mCounterLabel->mapToGlobal(QPoint(0,0)).y() );
			serviceStatusView->setGeometry( QRect( bottomRight - QPoint(sh.width(),sh.height()+4), sh ) );
			serviceStatusView->show();
		}
	}
	return false;
}

void MainWindow::updateFarmStatus( const Service & managerService )
{
	if( managerService.isRecord() && managerService.enabled() ) {
		mFarmStatusLabel->setText( " Farm Status: <font color=\"green\">Online</font> " );
	} else {
		mFarmStatusLabel->setText( " Farm Status: <font color=\"red\">Offline</font> [" + managerService.description() + "] " );
	}
}

