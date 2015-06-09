
/* $Author: brobison $
 * $LastChangedDate: 2007-06-18 11:27:47 -0700 (Mon, 18 Jun 2007) $
 * $Rev: 4632 $
 * $HeadURL: svn://brobision@66.93.150.126/blur/trunk/cpp/lib/assfreezer/src/jobsettingswidget.cpp $
 */

#include <qtimer.h>
#include <qfile.h>

#include "config.h"
#include "jobassignmentwindow.h"

JobAssignmentWindow::JobAssignmentWindow( QWidget * parent )
: QMainWindow( parent )
, mJaWidget( 0 )
{
	setAttribute( Qt::WA_DeleteOnClose, true );

	mJaWidget = new JobAssignmentWidget( this );
	setCentralWidget( mJaWidget );
	
	resize(600, 800);
}

JobAssignmentWindow::~JobAssignmentWindow()
{}

JobAssignmentWidget * JobAssignmentWindow::jaWidget() const
{
	return mJaWidget;
}

