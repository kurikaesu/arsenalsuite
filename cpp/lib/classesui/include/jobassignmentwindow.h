
/* $Author$
 * $LastChangedDate$
 * $Rev$
 * $HeadURL$
 */

#ifndef JOB_ASSIGNMENT_WINDOW_H
#define JOB_ASSIGNMENT_WINDOW_H

#include <qmainwindow.h>

#include "jobassignment.h"

#include "classesui.h"
#include "jobassignmentwidget.h"


class CLASSESUI_EXPORT JobAssignmentWindow : public QMainWindow
{
Q_OBJECT
public:
	JobAssignmentWindow( QWidget * parent=0 );
	~JobAssignmentWindow();

	JobAssignmentWidget * jaWidget() const;

protected:
	JobAssignmentWidget * mJaWidget;
};


#endif // JOB_ASSIGNMENT_WINDOW_H

