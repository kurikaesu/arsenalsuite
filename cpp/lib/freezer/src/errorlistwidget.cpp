
#include <qlayout.h>
#include <qmessagebox.h>
#include <qtoolbar.h>

#include "database.h"
#include "freezercore.h"
#include "process.h"

#include "busywidget.h"
#include "filteredit.h"
#include "modelgrouper.h"
#include "recordtreeview.h"
#include "viewcolors.h"

#include "afcommon.h"
#include "assfreezermenus.h"
#include "errorlistwidget.h"
#include "items.h"
#include "joblistwidget.h"
#include "mainwindow.h"
#include "threadtasks.h"

ErrorListWidget::ErrorListWidget( QWidget * parent )
: FreezerView( parent )
, mErrorTree(0)
, mErrorTaskRunning( false )
, mQueuedErrorRefresh( false )
, mToolBar( 0 )
, mErrorMenu( 0 )
, mLimit( 1000 )
, mErrorFilterEdit( 0 )
{
	RefreshErrorsAction = new QAction( "Refresh Errors", this );
	RefreshErrorsAction->setIcon( QIcon( ":/images/refresh" ) );

	connect( RefreshErrorsAction, SIGNAL( triggered(bool) ), SLOT( refresh() ) );

	mErrorTree = new RecordTreeView(this);
	QLayout * hbox = new QHBoxLayout(this);
	hbox->setMargin(0);
	hbox->addWidget(mErrorTree);

	/* ListView connections */
	connect( mErrorTree, SIGNAL( selectionChanged(RecordList) ), SLOT( errorListSelectionChanged() ) );

	mErrorTree->setContextMenuPolicy( Qt::CustomContextMenu );
	connect( mErrorTree, SIGNAL( customContextMenuRequested( const QPoint & ) ), SLOT( showErrorPopup( const QPoint & ) ) );
	
	RecordSuperModel * em = new ErrorModel(mErrorTree);
	// Enable grouping
	connect( em->grouper(), SIGNAL( groupingChanged(bool) ), SLOT( slotGroupingChanged(bool) ) );
	
	mErrorFilterEdit = new FilterEdit( this, JobError::c.Message, "Error Text Filter:" );
	connect( mErrorFilterEdit, SIGNAL( filterChanged( const Expression & ) ), SLOT( refresh() ) );

	em->setAutoSort( true );
	mErrorTree->setModel( em );

	IniConfig temp;
	restore(temp);
}

ErrorListWidget::~ErrorListWidget()
{
}

QString ErrorListWidget::viewType() const
{
	return "ErrorList";
}

RecordTreeView * ErrorListWidget::errorTreeView() const
{
	return mErrorTree;
}

void ErrorListWidget::save( IniConfig & ini, bool forceFullSave )
{
	saveErrorView(mErrorTree,ini);
	FreezerView::save(ini,forceFullSave);
}

void ErrorListWidget::restore( IniConfig & ini, bool forceFullRestore )
{
	setupErrorView(mErrorTree,ini);
	FreezerView::restore(ini,forceFullRestore);
}

void ErrorListWidget::customEvent( QEvent * evt )
{
	if( ((ThreadTask*)evt)->mCancel )
		return;
		
	switch( (int)evt->type() ) {
		case ERROR_LIST:
		{
			ErrorListTask * elt = (ErrorListTask*)evt;
			((ErrorModel*)mErrorTree->model())->setErrors( elt->mReturn, elt->mJobs, elt->mJobServices );

			mErrorTree->busyWidget()->stop();
			
			mErrorTaskRunning = false;
			if( mQueuedErrorRefresh ) {
				mQueuedErrorRefresh = false;
				refresh();
			} else
				clearStatusBar();
			break;
		}
		default:
			break;
	}
}

void ErrorListWidget::doRefresh()
{
	FreezerView::doRefresh();
	bool needStatusBarMsg = false, needErrorListTask = false;

	if( mErrorTaskRunning )
		mQueuedErrorRefresh = true;
	else
		needStatusBarMsg = needErrorListTask = true;

	if( needStatusBarMsg )
		setStatusBarMessage( "Refreshing Error List..." );

	if( needErrorListTask ) {
		mErrorTaskRunning = true;
		mErrorTree->busyWidget()->start();
		FreezerCore::addTask( new ErrorListTask( this, mJobFilter, mHostFilter, mServiceFilter, mJobTypeFilter, mMessageFilter, true, mLimit, mErrorFilterEdit->expression() ) );
		FreezerCore::wakeup();
	}
}

void ErrorListWidget::errorListSelectionChanged()
{
	JobErrorList errors = mErrorTree->selection();
	if( errors.size() )
		setStatusBarMessage( QString::number( errors.size() ) + " Errors Selected" );
	else
		clearStatusBar();
}

QToolBar * ErrorListWidget::toolBar( QMainWindow * mw )
{
	if( !mToolBar ) {
		mToolBar = new QToolBar( mw );
		mToolBar->addAction( RefreshErrorsAction );
		mToolBar->addWidget( mErrorFilterEdit );
	}
	return mToolBar;
}

void ErrorListWidget::populateViewMenu( QMenu * viewMenu )
{
	Q_UNUSED(viewMenu);
}

void ErrorListWidget::showJobs()
{
	MainWindow * mw = qobject_cast<MainWindow*>(window());
	if( mw ) {
		JobListWidget * jobList = new JobListWidget(mw);
		jobList->setJobList( JobErrorList(mErrorTree->selection()).jobs().unique() );
		mw->insertView(jobList);
		mw->setCurrentView(jobList);
	}
}

void ErrorListWidget::applyOptions()
{
	options.mErrorColors->apply(mErrorTree);
	mErrorTree->setFont( options.jobFont );

	QPalette p = mErrorTree->palette();
	ColorOption * co = options.mErrorColors->getColorOption("Default");
	p.setColor(QPalette::Active, QPalette::AlternateBase, co->bg.darker(120));
	p.setColor(QPalette::Inactive, QPalette::AlternateBase, co->bg.darker(120));
	mErrorTree->setPalette( p );

	mErrorTree->update();
}

void ErrorListWidget::slotGroupingChanged(bool grouped)
{
	mErrorTree->setRootIsDecorated(grouped);
}

void ErrorListWidget::showErrorPopup( const QPoint & point )
{
	if( !mErrorMenu )
		mErrorMenu = new FreezerErrorMenu( this, mErrorTree->selection(), mErrorTree->model()->rootList() );
	else
		mErrorMenu->setErrors( mErrorTree->selection(), mErrorTree->model()->rootList() );
	mErrorMenu->popup( mErrorTree->mapToGlobal(point) );
}

JobList ErrorListWidget::jobFilter() const
{
	return mJobFilter;
}

void ErrorListWidget::setJobFilter( JobList jobFilter )
{
	mJobFilter = jobFilter;
	refresh();
}

