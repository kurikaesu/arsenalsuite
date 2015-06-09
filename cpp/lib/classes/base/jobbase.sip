
	static bool updateJobStatuses( JobList jobs, const QString & jobStatus, bool resetTasks, bool restartHosts );

	void changeFrameRange( QList<int>, JobOutput output = JobOutput(), bool changeCancelledToNew = true );

	void changePreassignedTaskList( HostList hosts, bool changeCancelledToNew = true );

	void addHistory( const QString & message );
