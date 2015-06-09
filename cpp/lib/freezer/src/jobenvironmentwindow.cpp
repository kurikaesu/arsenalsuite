
/* $Author$
 * $LastChangedDate: 2007-06-18 11:27:47 -0700 (Mon, 18 Jun 2007) $
 * $Rev: 4632 $
 * $HeadURL: svn://brobision@66.93.150.126/blur/trunk/cpp/lib/assfreezer/src/jobsettingswidget.cpp $
 */

#include "jobenvironmentwindow.h"

JobEnvironmentWindow::JobEnvironmentWindow( QWidget * parent )
: QDialog( parent )
{
	setupUi( this );
	resize(600, 800);
}

QString JobEnvironmentWindow::environment()
{
	return textEdit->toPlainText();
}

void JobEnvironmentWindow::setEnvironment(const QString & env)
{
	textEdit->setText( env );
}

