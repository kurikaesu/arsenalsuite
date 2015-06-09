
#ifndef JOB_ASSIGNMENT_WIDGET_H
#define JOB_ASSIGNMENT_WIDGET_H

#include <qwidget.h>

#include "jobassignment.h"

#include "classesui.h"

#include "ui_jobassignmentwidgetui.h"


class CLASSESUI_EXPORT JobAssignmentWidget : public QWidget, public Ui::JobAssignmentWidgetUI
{
Q_OBJECT
public:
	JobAssignmentWidget( QWidget * parent=0 );
	~JobAssignmentWidget();

	void setJobAssignment(const JobAssignment &);
	JobAssignment jobAssignment();

public slots:
	void refresh();

protected slots:
	void doRefresh();

protected:
	QTimer * mRefreshTimer;
	bool mRefreshRequested;
	JobAssignment mJobAssignment;
	QString mLogRoot;
};

#endif // JOB_ASSIGNMENT_WIDGET_H
