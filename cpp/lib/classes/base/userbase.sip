

	static MappedRecord currentUser();
	static void setCurrentUser( const QString & );

    static bool isUserLoggedIn( const QString & );
	
	static MappedRecord activeByUserName( const QString & );

	static MappedRecord setupProjectUser( Project p, Client c, bool ftp, bool web );
	
	static bool hasPerms( const QString &, bool modify = false, const Project & project = Project() );
	static void permAction( QAction *, const QString &, bool modify = false );

	/* Roles */
	MappedRecordList roles() const;
	void addRole( const AssetType & );
	void removeRole( const AssetType & );

	/* ToolBar Elements */
	MappedRecordList toolBarElements() const;
	void addToolBarElement( const Element & );
	void removeToolBarElement( const Element & );

	bool relatedElement( const Element &, bool recurse=false );

	static uint nextUID();
	static uint nextGID();

	void setDefaultNotificationMethods( NotificationMethodList );
	MappedRecordList defaultNotificationMethods() const;
