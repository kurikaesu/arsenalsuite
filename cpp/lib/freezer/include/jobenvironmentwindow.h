
/* $Author$
 * $LastChangedDate$
 * $Rev$
 * $HeadURL$
 */

#ifndef JOB_ENVIRONMENT_WINDOW_H
#define JOB_ENVIRONMENT_WINDOW_H

#include <qmainwindow.h>

#include "ui_jobenvironmentwindowui.h"

class JobEnvironmentWindow : public QDialog, public Ui::JobEnvironmentWindowUI
{
Q_OBJECT
public:
	JobEnvironmentWindow( QWidget * parent=0 );

	void setEnvironment(const QString &);
	QString environment();

};

#endif // JOB_ENVIRONMENT_WINDOW_H

