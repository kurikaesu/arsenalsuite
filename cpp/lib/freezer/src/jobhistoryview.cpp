
#include "blurqt.h"
#include "iniconfig.h"

#include "recordsupermodel.h"

#include "afcommon.h"
#include "viewcolors.h"
#include "jobhistoryview.h"

static const ColumnStruct job_history_columns [] =
{
	{ "Key", 				"KeyColumn", 		50, 	1, true },
	{ "Job", 				"JobColumn", 		300, 	2, false },
	{ "Host", 				"HostColumn", 		70, 	3, false },
	{ "Message", 			"MessageColumn", 	600, 	5, false },
	{ "User", 				"UserColumn", 		80, 	4, false },
	{ "Created",			"CreatedColumn", 	80,		6, false },
	{ 0, 0, 0, 0, 0 }
};

JobHistoryView::JobHistoryView( QWidget * parent )
: RecordTreeView ( parent )
, mModel( 0 )
{
	setAlternatingRowColors(false);
	setVerticalScrollMode( ScrollPerPixel );
	setHorizontalScrollMode( ScrollPerPixel );

	options.mErrorColors->apply(this);
}

JobHistoryView::~JobHistoryView()
{
	IniConfig & ini = userConfig();
	ini.pushSection( "JobHistoryView" );
	//saveTreeView( ini, job_history_columns );
	ini.popSection();
}

bool JobHistoryView::event ( QEvent * event )
{
	if( event->type() == QEvent::PolishRequest )
		setupModel();
	return RecordTreeView::event(event);
}

void JobHistoryView::setupModel()
{
	if( !mModel ) {
		mModel = new RecordSuperModel(this);
		RecordDataTranslator * rdt = new RecordDataTranslator(mModel->treeBuilder());
		rdt->setRecordColumnList(QStringList() << "key" << "fkeyjob" << "fkeyhost" << "message" << "fkeyuser" << "created");
		setModel( mModel );
		IniConfig & ini = userConfig();
		ini.pushSection( "JobHistoryView" );
		setupTreeView( ini, job_history_columns );
		ini.popSection();
	}
}

void JobHistoryView::setHistory( JobHistoryList history )
{
	setupModel();
	mJobs = JobList();
	mModel->setRootList(history);
}

JobHistoryList JobHistoryView::history()
{
	return mModel->rootList();
}

void JobHistoryView::setJobs( JobList jobs )
{
	setHistory(jobs.jobHistories());
	mJobs = jobs;
}

JobList JobHistoryView::jobs()
{
	return mJobs;
}

void JobHistoryView::applyOptions()
{
	options.mErrorColors->apply(this);
}
