
	MappedRecord findDefaultTemplate( const Project & project = Project() );

	MappedRecord construct();

	MappedRecord getGroupElement( const Element & );

	/* Returns the AssetType for a character */
	static MappedRecord character();

	/* Returns the AssetType for a prop */
	static MappedRecord prop();

	/* Returns the AssetType for an environment */
	static MappedRecord environment();

	static MappedRecord camera();

	static MappedRecord light();


