
	
	QString fileName() const;
	bool setFileName( const QString & );
	void checkForUpdates();
	bool doesTrackFile( const QString & filePath );
	QDateTime updated( bool last = true ) const;
	
	QStringList files() const;

//	RFTFrame firstFrame() const;
//	RFTFrame lastFrame() const;
//	RFTFrame frame( int ) const;
		
	QString sortString( int frame ) const;

	QString displayNumber( int frame ) const;

	QString filePath( int frame ) const;
	QString fileName( int frame ) const;

	void setFilePath( const QString & );

	void fillFrames( bool overwrite=false );
	
	void fillFrame( int frame ) const;
	
	static QString timeCode( int frame, int fps );

	void deleteFrame( int frame ) const;

