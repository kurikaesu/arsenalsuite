
	static ServerFileAction remove( const QString & path );

	static ServerFileAction move( const QString & source, const QString & dest );

	enum Status {
		New,
		Complete,
		Error
	};

	Status status();
	ServerFileAction & setStatus( Status status );

	enum Type {
		Delete,
		Move
	};

	Type type();
	ServerFileAction & setType( Type type );
