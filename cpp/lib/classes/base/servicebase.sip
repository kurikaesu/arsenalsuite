

	bool pulse( bool autoCreate = true );

	static MappedService ensureServiceExists( const QString & name );

	MappedHostService byHost( const Host & host = Host::currentHost(), bool create = true );
