

class JobListWidget : FreezerView
{
%TypeHeaderCode
#include <joblistwidget.h>
%End
public:
	JobListWidget( QWidget * parent );
	~JobListWidget();

	virtual QString viewType() const;

	/// access to the GL widget where image playback occurs
	//ImageView * imageView() const;

	JobSettingsWidget * jobSettingsWidget() const;

//	const JobFilter & jobFilter();
//	void setJobFilter( const JobFilter & );

	// If this view is set to show a specific list of jobs with setJobList
	// this will return those jobs until clearJobList is called.
	// Job filters are ignored while this list is not empty
	JobList jobList() const;

	/// don't show jobs with fkeyProject in hiddenProjects
	void setHiddenProjects( ProjectList hiddenProjects );

	/// show jobs with fkeyElement in ElementList
	void setElementList( ElementList );
	
	/// show jobs with status in the list
	void setStatusToShow( QStringList );

	QAction* RefreshAction;
	QAction* RestartAction;
	QAction* KillAction;
	QAction* PauseAction;
	QAction* ResumeAction;
	QAction* ShowOutputAction;
	QAction* ShowMineAction;
	QAction* FrameCyclerAction;
	QAction* PdPlayerAction;
	QAction* ClearErrorsAction;
	QAction* DependencyTreeEnabledAction;
	QAction* NewViewFromSelectionAction;
	QAction* ExploreJobFile;

//	JobFilterEdit * mJobFilterEdit;

	ProjectList activeProjects();

	bool isDependencyTreeEnabled() const;

	Job currentJob() const;

	virtual QToolBar * toolBar( QMainWindow * );
	virtual void populateViewMenu( QMenu * );

	RecordTreeView * jobTree();

public slots:
	/// show these jobs
	void setJobList( JobList );

	/// called to populate the tab according to the selected jobs
	void currentTabChanged(bool refresh=true);
	/// called to populate the tab according to the selected jobs
	void refreshCurrentTab();

	void setDependencyTreeEnabled( bool dependencyTreeEnabled, bool allowRefresh = true );
	void jobDependenciesExpanded( const QModelIndex & );

	/// refreshes the frame list from the database
	/// @param jobChange - set to true to clear the ImageView's cache
	void refreshFrameList( bool jobChange = true );
	void refreshErrorList();

	void currentJobChanged();
	void jobListSelectionChanged();

	void frameListSelectionChanged();
	void frameSelected(const Record &);

	/// selected jobs are told to re-render all frames
	void restartJobs();
	/// selected jobs are told to resume burning
	void resumeJobs();
	/// selected jobs are told to stop burning, all running tasks stop immediately
	void pauseJobs();
	/// selected jobs are told to stop burning, all running tasks stop immediately, job is deleted
	void deleteJobs();
	void updateSelectedJobs(const QString & jobStatus, bool resetTasks);
	/// toggles whether to only show jobs owned by current user
	void showMine(bool);
	/// selected jobs have all errors cleared
	void clearErrors();
	void jobFilterChanged( const Expression & );

	/// run playback tool for output of selected jobs
	void frameCycler();
	void pdPlayer();

	/// shot output of selected jobs in file browser
	void outputPathExplorer();

	void exploreJobFile();

	/// should select frameNumber in the frame list, currently does nothing
	/// @param frameNumber
	void changeFrameSelection(int);

	/// Used to refresh view when services column visibility goes from false to true
	void jobTreeColumnVisibilityChanged( int column, bool visible );

	void showJobPopup(const QPoint &);
	void showFramePopup(const QPoint &);
	void showErrorPopup(const QPoint &);

	void showJobInfo();
	void showJobStatistics();
	void clearJobList();
	void setJobPriority();
	void saveCannedBatchJob();

	void createNewViewFromSelection();

	void setFrameCacheStatus(int,int);

	void setLimit();

	void applyOptions();

	void populateErrorTreeHeaderMenu( QMenu * );
	void setShowClearedErrors( bool );

};
