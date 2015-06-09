
	MappedRecord findSequence( const QString & name );

	MappedRecord findShot( const QString & sequenceName, const QString & shotName );

	MappedRecord findShotFromPath( const QString & path );

	static MappedRecord findProjectFromPath( const QString & path );

	MappedRecordList shots();

	MappedRecordList sequences();
