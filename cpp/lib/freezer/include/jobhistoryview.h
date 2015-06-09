
#ifndef JOB_HISTORY_VIEW_H
#define JOB_HISTORY_VIEW_H

#include <recordtreeview.h>

#include "jobhistory.h"
#include "job.h"

class RecordSuperModel;

class JobHistoryView : public RecordTreeView
{
Q_OBJECT
public:
	JobHistoryView( QWidget * parent = 0 );
	~JobHistoryView();

public slots:
	void setHistory( JobHistoryList history );
	JobHistoryList history();

	// Calls setHistoryList with all the history from jobs
	void setJobs( JobList jobs );
	JobList jobs();

	void applyOptions();

protected:
	void setupModel();
	bool event ( QEvent * event );
	
	JobList mJobs;
	RecordSuperModel * mModel;
};

#endif // JOB_HISTORY_VIEW_H

