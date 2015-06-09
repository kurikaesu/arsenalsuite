
	
	QString fileName() const;
	bool setFileName( const QString & );
	
	QString path() const;
	void setPath( const QString & );
	
	void checkForUpdates();
	
	bool doesTrackFile( const QString & );
	
	QStringList fileNames() const;

	QStringList oldFileNames() const;
	void setOldFileNames( const QStringList & );

	bool copyToFinal( QString * errorMsg=0 ) const;
	QString finalPath() const;
	QString finalFilePath() const;
	
	QString getFinalFileName() const;
	
	uint version() const;
	void setVersion( uint version );
	
	uint iteration() const;
	void setIteration( uint iteration );
	
	bool autoFinal() const;
	void setAutoFinal( bool autoFinal );
	
	QString fileNameTemplate() const;
	void setFileNameTemplate( const QString & );
	
	bool isLinked() const;
	//
	// Parses the fileName to get the version and iteration
	// Only returns true if the filename matches this fileversion's
	// prefix and if the version and iteration can be parsed out
	bool parseVersion( const QString & fileName, int * version=0, int * iteration=0 );

	QString assembleVersion( int version, int iteration );
	
	// Returns all of the elements that are using this
	// file tracker. (Linked filetrackers' elements, and
	// the one non-linked filetracker's element).
	MappedRecordList elements() const;
	
	// Returns all the elements for the tracker
	// that tracks 'file'
	static MappedRecordList elementsFromPath( const QString & file );

