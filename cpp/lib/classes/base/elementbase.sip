
	bool isTask() const;

	// Assigned user stuff
	MappedRecordList users( bool recursive = false ) const;

	QStringList userStringList() const;

	MappedRecord addUser( const User & );

	void removeUser( const User & );

	void setUsers( EmployeeList );

	bool hasUser ( const User & );

//	static Element recordByElementAndName( const Element &, const QString & );

//	float daysSpent() const;

	// Template Stuff
	static MappedRecord createFromTemplate( const AssetTemplate &, RecordList & created );
	void copyTemplateRecurse( const Element &, RecordList & created );
	void createFileDependancies( const Element & dependant, bool recursive );

	// File Tracker Stuff
	MappedRecordList trackers( bool recursive = true ) const;
	MappedRecordList findTrackers( const QRegExp &, bool recursive = true ) const;
	MappedRecord tracker( const QString & key ) const;
	
	QString displayName( bool needContext = false ) const;
	QString displayPath() const;
	QString prettyPath() const;

	/***************************************************************
		Path functions
	****************************************************************/
	QString uri() const;
	
	MappedRecord childFromUri( const QString & dotPath );
	
	static MappedRecord fromUri( const QString & dotPath );

	/// Returns the ProjectStorage record for this project with storageName
	MappedRecord storageByName( const QString & storageName ) const;
	MappedRecord defaultStorage() const;
	MappedRecord storageByDriveLetter( const QString & driveLetter ) const;

	MappedRecord pathTracker( const ProjectStorage & ps = ProjectStorage() );

	QString path( const ProjectStorage & ps = ProjectStorage() );
	QString path( const QString & storageName );
	
	bool setPath( const QString & path, const ProjectStorage & storage = ProjectStorage() );
	bool setPath( const QString & path, const QString & storageName );

	static MappedRecord fromPath( const QString & path, bool matchClosest = false );

	QString setCachedPath( const QString & ) const;
	static void invalidatePathCache();

	QString elementPath() const;
	static MappedRecord fromElementPath( const QString & );
	
//	static bool import( const QString & fileName );

//	bool importRecurse( const QString & fileName, int level, RecordList * created = 0 ) const;

//	static MappedRecord fromDir( const QString & dir );

	bool createPath( bool createParents );
	
	void createAllPaths( bool );
	
	static QString driveToUnix( const QString & drive );

	/*********************************************************
		Drag and drop
	**********************************************************/
	void dropped( QWidget * window, const Record &, Qt::DropAction );
	//void copy( const Element & copyParent );
	void move( const Element & newParent );

	/**********************************************************
		Relations
	***********************************************************/
	MappedRecordList dependencies() const;
	MappedRecordList dependants() const;

	void setDependencies( ElementList elist );
	void setDependants( ElementList elist );
	
	MappedRecordList children( const AssetType assetType, bool recursive=false ) const;
	MappedRecordList children( bool recursive = false ) const;

	MappedRecord ancestorByAssetType( const AssetType & at );

	MappedRecordList coordinators() const;
	MappedRecordList producers() const;
	MappedRecordList supervisors() const;
	
	QStringList propertyNames() const;

	QVariant getProperty( const QString & propertyName, const QVariant & defaultValue = QVariant(), bool storeDefault = false ) const;
	
	QVariant setProperty( const QString & propertyName, const QVariant & value );
