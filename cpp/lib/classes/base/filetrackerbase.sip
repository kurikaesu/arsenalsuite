

	bool pathMatchesTemplate() const;
	bool fileNameMatchesTemplate() const;
	bool matchesTemplate() const;

	bool doesTrackFile( const QString & filePath ) const;
	
	// Generates the filename from the path template
	QString generateFileName() const;

	QString fileName() const;
	bool setFileName( const QString & fn );

	// Generates the path from the path template
	QString generatePath() const;

	QString path() const;
	void setPath( const QString & path );

	QString setCachedPath( const QString & path ) const;
	static void invalidatePathCache();

	void checkForUpdates();
	
	QString filePath() const;
	void setFilePath( const QString & filePath );

	QDateTime updated( bool last = true ) const;
	QDateTime firstUpdated() const;
	QDateTime lastUpdated() const;
	
	// Returns all FileTrackers that have files in 'dir'
	static MappedRecordList fromDirPath( const QString & dir );
	
	// Returns the FileTracker that tracks 'file'
	static MappedRecord fromPath( const QString & file );
	
	bool needsUpdate() const;
	
	MappedRecordList inputs() const;
	MappedRecordList outputs() const;


