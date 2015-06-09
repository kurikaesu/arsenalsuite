
#ifndef ERROR_LIST_WIDGET_H
#define ERROR_LIST_WIDGET_H

#include <qstring.h>

#include "host.h"
#include "job.h"
#include "jobtype.h"
#include "service.h"

#include "assfreezerview.h"

class QAction;
class QMenu;

class RecordTreeView;

class FreezerErrorMenu;
class FilterEdit;

class FREEZER_EXPORT ErrorListWidget : public FreezerView
{
Q_OBJECT
public:
	ErrorListWidget(QWidget * parent);
	~ErrorListWidget();

	virtual QString viewType() const;

	QAction* RefreshErrorsAction;

	virtual QToolBar * toolBar( QMainWindow * );
	virtual void populateViewMenu( QMenu * );

	RecordTreeView * errorTreeView() const;
	
	JobList jobFilter() const;
public slots:

	void errorListSelectionChanged();
	
	/// Opens a Job View and selects the jobs currently assigned to the selected hosts
	void showJobs();

	void applyOptions();

	void slotGroupingChanged(bool);
	
	void showErrorPopup( const QPoint & );
	
	void setJobFilter( JobList );
protected:
	/// refreshes the host list from the database
	void doRefresh();

	void save( IniConfig & ini, bool = false );
	void restore( IniConfig & ini, bool = false );

	void customEvent( QEvent * evt );

	RecordTreeView * mErrorTree;

	bool mErrorTaskRunning;
	bool mQueuedErrorRefresh;

	QToolBar * mToolBar;

public:
	FreezerErrorMenu * mErrorMenu;
	
	HostList mHostFilter;
	JobList mJobFilter;
	ServiceList mServiceFilter;
	JobTypeList mJobTypeFilter;
	QString mMessageFilter;
	int mLimit;
	FilterEdit * mErrorFilterEdit;
	friend class AssfreezerErrorMenu;
};

#endif // ERROR_LIST_WIDGET_H
