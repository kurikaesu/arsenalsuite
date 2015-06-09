
#include <qtimer.h>
#include <qfile.h>

#include "config.h"
#include "jobassignmentwidget.h"

JobAssignmentWidget::JobAssignmentWidget( QWidget * parent )
: QWidget( parent )
, mRefreshRequested( false )
{
	setupUi( this );

	setAttribute( Qt::WA_DeleteOnClose, true );

//	connect(mRefreshButton, SIGNAL(pressed()), SLOT(refresh()));
	resize(600, 800);
	mLogRoot = Config::getString("assburnerLogRootDir", "");
}

JobAssignmentWidget::~JobAssignmentWidget()
{}

JobAssignment JobAssignmentWidget::jobAssignment()
{
	return mJobAssignment;
}

void JobAssignmentWidget::setJobAssignment(const JobAssignment & ja)
{
	mJobAssignment = ja;
	refresh();
}

void JobAssignmentWidget::refresh()
{
	if( !mRefreshRequested ) {
		mRefreshRequested = true;
		QTimer::singleShot( 0, this, SLOT( doRefresh() ) );
	}
}

void JobAssignmentWidget::doRefresh()
{
	mRefreshRequested = false;
	mJobAssignment.reload();
	mCommandEdit->setText( mJobAssignment.command() );
	if( mLogRoot.isEmpty() ) {
		mLogText->document()->setPlainText( mJobAssignment.stdOut() );
	} else {
		QFile read(mJobAssignment.stdOut());
		if( !read.open(QIODevice::ReadOnly | QIODevice::Text) )
			return;
		mLogText->document()->setPlainText( read.readAll() );
	}
	mLogText->moveCursor(QTextCursor::End);
}

