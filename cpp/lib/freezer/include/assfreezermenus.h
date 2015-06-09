
#ifndef ASSFREEZER_MENUS_H
#define ASSFREEZER_MENUS_H

#include <qmenu.h>

#include "jobtask.h"
#include "joberror.h"
#include "service.h"

#include "afcommon.h"

class QAction;

class FreezerView;
class JobListWidget;
class HostListWidget;
class ErrorListWidget;
class JobViewerPlugin;
class HostViewerPlugin;
class FrameViewerPlugin;
class MultiFrameViewerPlugin;

class FREEZER_EXPORT FreezerMenuPlugin
{
public:
	virtual ~FreezerMenuPlugin(){}
	
	virtual void executeMenuPlugin( QMenu * )=0;
};

class FREEZER_EXPORT FreezerMenuFactory
{
public:
	static FreezerMenuFactory * instance();
	
	void registerMenuPlugin( FreezerMenuPlugin * plugin, const QString & menuName );
	QList<QAction*> aboutToShow( QMenu * menu, bool addPreSep = false, bool addPostSep = false );
	
	QMap<QString,QList<FreezerMenuPlugin*> > mPlugins;
};

class FREEZER_EXPORT FreezerMenu : public QMenu
{
Q_OBJECT
public:
	FreezerMenu( QWidget * parent, const QString & title = QString() );

public slots:
	virtual void slotCurrentViewChanged( FreezerView * );
	virtual void slotAboutToShow() = 0;
	virtual void slotActionTriggered( QAction * ) {}
};

template<class BASE=FreezerMenu> class FREEZER_EXPORT JobListMenu : public BASE
{
public:
	JobListMenu(JobListWidget * jobList, const QString & title = QString())
	: BASE(jobList, title)
	, mJobList(jobList)
	{}

	JobListWidget * mJobList;
protected:
	void slotCurrentViewChanged( FreezerView * );
};

template<class BASE=FreezerMenu> class FREEZER_EXPORT HostListMenu : public BASE
{
public:
	HostListMenu(HostListWidget * hostList, const QString & title = QString())
	: BASE(hostList, title)
	, mHostList(hostList)
	{}
	
	HostListWidget * mHostList;
protected:
	void slotCurrentViewChanged( FreezerView * );
};

template<class BASE=FreezerMenu> class FREEZER_EXPORT ErrorListMenu : public BASE
{
public:
	ErrorListMenu(ErrorListWidget * errorList, const QString & title = QString())
	: BASE(errorList, title)
	, mErrorList(errorList)
	{}
	
	ErrorListWidget * mErrorList;
protected:
	void slotCurrentViewChanged( FreezerView * );
};

class FREEZER_EXPORT StatusFilterMenu : public JobListMenu<>
{
Q_OBJECT
public:
	StatusFilterMenu(JobListWidget *);

	void slotAboutToShow();
	void slotActionTriggered(QAction*);
	void updateActionStates();

protected:
	bool mStatusActionsCreated;
	QAction * mStatusShowAll, * mStatusShowNone;
	QList<QAction*> mStatusActions;
};

class FREEZER_EXPORT ProjectFilterMenu : public JobListMenu<>
{
Q_OBJECT
public:
	ProjectFilterMenu(JobListWidget *);

	void slotAboutToShow();
	void slotActionTriggered(QAction*);
	void updateActionStates();

protected:
	QAction * mProjectShowAll, * mProjectShowNone, * mProjectShowNonProject;
	QList<QAction*> mProjectActions;
	bool mProjectActionsCreated;
};

class FREEZER_EXPORT JobTypeFilterMenu : public FreezerMenu
{
Q_OBJECT
public:
	JobTypeFilterMenu(FreezerView *, const QString & title = QString());

	void slotAboutToShow();
	void slotActionTriggered(QAction*);
	void updateActionStates();

	virtual JobTypeList jobTypeFilter() const = 0;
	virtual void setJobTypeFilter( JobTypeList ) = 0;
	virtual JobTypeList activeJobTypes() const = 0;
protected:
	bool mJobTypeActionsCreated;
	QAction * mJobTypeShowAll, * mJobTypeShowNone;
	QList<QAction*> mJobTypeActions;
};

class FREEZER_EXPORT JobListJobTypeFilterMenu : public JobListMenu<JobTypeFilterMenu>
{
Q_OBJECT
public:
	JobListJobTypeFilterMenu(JobListWidget * jobList);
	
	virtual JobTypeList jobTypeFilter() const;
	virtual void setJobTypeFilter( JobTypeList );
	virtual JobTypeList activeJobTypes() const;
};

class FREEZER_EXPORT ErrorListJobTypeFilterMenu : public ErrorListMenu<JobTypeFilterMenu>
{
Q_OBJECT
public:
	ErrorListJobTypeFilterMenu(ErrorListWidget * errorList);
	
	virtual JobTypeList jobTypeFilter() const;
	virtual void setJobTypeFilter( JobTypeList );
	virtual JobTypeList activeJobTypes() const;
};

class FREEZER_EXPORT ServiceFilterMenu : public FreezerMenu
{
Q_OBJECT
public:
	ServiceFilterMenu(FreezerView * view, const QString & title);
	
	void slotAboutToShow();
	void slotActionTriggered(QAction*);
	void updateActionStates();

	virtual ServiceList serviceFilter() const = 0;
	virtual void setServiceFilter( ServiceList ) = 0;
	virtual ServiceList activeServices() const = 0;
protected:
	bool mJobServiceActionsCreated;
	QAction * mJobServiceShowAll, * mJobServiceShowNone;
	QList<QAction*> mJobServiceActions;
};

class FREEZER_EXPORT JobServiceFilterMenu : public JobListMenu<ServiceFilterMenu>
{
Q_OBJECT
public:
	JobServiceFilterMenu(JobListWidget * jobList);
	
	ServiceList serviceFilter() const;
	ServiceList activeServices() const;
	void setServiceFilter( ServiceList );
};

class FREEZER_EXPORT HostServiceFilterMenu : public HostListMenu<ServiceFilterMenu>
{
Q_OBJECT
public:
	HostServiceFilterMenu(HostListWidget * hostList);

	ServiceList serviceFilter() const;
	ServiceList activeServices() const;
	void setServiceFilter( ServiceList );
};

class FREEZER_EXPORT ErrorServiceFilterMenu : public ErrorListMenu<ServiceFilterMenu>
{
Q_OBJECT
public:
	ErrorServiceFilterMenu(ErrorListWidget * errorList);

	ServiceList serviceFilter() const;
	ServiceList activeServices() const;
	void setServiceFilter( ServiceList );
};

class FREEZER_EXPORT FreezerJobMenu : public JobListMenu<>
{
Q_OBJECT
public:
	FreezerJobMenu(JobListWidget *);

	void slotAboutToShow();
	void slotActionTriggered( QAction * );

protected:
	void modifyFrameRange( Job );
	QAction * mRemoveDependencyAction;
	QAction * mShowHistoryAction;
	QAction * mSubmitVideoMakerAction;
	QAction * mSetJobKeyListAction;
	QAction * mClearJobKeyListAction;
	QAction * mModifyFrameRangeAction;

	QMap<QAction *, JobViewerPlugin *> mJobViewerActions;
};

class FREEZER_EXPORT HostPluginMenu : public HostListMenu<>
 {
public:
	HostPluginMenu(HostListWidget * hostList);

	void slotAboutToShow();
	void slotActionTriggered(QAction*);
protected:
	QMap<QAction *, HostViewerPlugin *> mHostPluginActions;
};

class FREEZER_EXPORT CannedBatchJobMenu : public HostListMenu<>
{
Q_OBJECT
public:
	CannedBatchJobMenu(HostListWidget * hostList);

	void slotAboutToShow();
	void slotActionTriggered(QAction*);
};

class FREEZER_EXPORT TailServiceLogMenu : public HostListMenu<>
{
Q_OBJECT
public:
	TailServiceLogMenu(HostListWidget * hostList);

	void slotAboutToShow();
	void slotActionTriggered(QAction*);

protected:
};

class FREEZER_EXPORT FreezerHostMenu : public HostListMenu<>
{
Q_OBJECT
public:
	FreezerHostMenu(HostListWidget * hostList);

	void slotAboutToShow();
	void slotActionTriggered(QAction*);
protected:
	QAction * mShowHistoryAction;
	QMap<QAction *, HostViewerPlugin *> mHostViewerActions;
	QAction * mNewHostAction, * mEditHostsAction, * mRemoveHostsAction;
	QAction * mCancelBatchJobTasksAction;
};

class FREEZER_EXPORT FreezerTaskMenu : public JobListMenu<>
{
Q_OBJECT
public:
	FreezerTaskMenu(JobListWidget *);

	void slotAboutToShow();
	void slotActionTriggered(QAction*);
protected:
	QAction * mInfoAction, * mRerenderFramesAction, * mSuspendFramesAction, 
	        * mCancelFramesAction, * mShowLogAction, * mCopyCommandAction,
			* mSelectHostsAction, * mVncHostsAction, * mShowHistoryAction;
	JobTaskList mTasks;
	QMap<QAction *, JobViewerPlugin *> mJobViewerActions;
	QMap<QAction *, HostViewerPlugin *> mHostViewerActions;
	QMap<QAction *, FrameViewerPlugin *> mFrameViewerActions;
	QMap<QAction *, MultiFrameViewerPlugin *> mMultiFrameViewerActions;
};

/* This menu is used by the errors tab in the Job List View(JobListWidget), and the main
 * list view in the Error List View(ErrorListWidget).  It customizes itself depending
 * on the context by checking the type of it's parent widget.
 */
class FREEZER_EXPORT FreezerErrorMenu : public FreezerMenu
{
Q_OBJECT
public:
	FreezerErrorMenu(QWidget *, JobErrorList selection, JobErrorList all);

	void setErrors( JobErrorList selection, JobErrorList allErrors );
	void slotAboutToShow();
	void slotActionTriggered(QAction*);

protected:
	JobErrorList mSelection, mAll;
	QAction 
	 * mClearSelected,
	 * mClearAll,
	 * mCopyText,
	 * mShowLog,
	 * mSelectHosts,
	 * mRemoveHosts,
	 * mClearHostErrorsAndOffline,
	 * mClearHostErrors,
	 * mVncHostsAction,
	 * mShowErrorInfo;
	
	// Error view specific menus and actions
	QAction
	 * mSetJobKeyListAction,
	 * mClearJobKeyListAction,
	 * mSetLimitAction;

	ErrorServiceFilterMenu * mServiceFilterMenu;
	ErrorListJobTypeFilterMenu * mJobTypeFilterMenu;
	
	QMap<QAction *, HostViewerPlugin *> mHostViewerActions;
	QMap<QAction *, FrameViewerPlugin *> mFrameViewerActions;
};

#endif // FREEZER_MENUS_H
