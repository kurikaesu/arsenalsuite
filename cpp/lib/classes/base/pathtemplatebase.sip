
/*
	Template replacements

	[Name] - the name of the element/asset
	[ParentPath] - the path of the parent asset
*/

MappedRecord createTracker( const Element & parent );

QString getPath( const Element & element, const ProjectStorage & storage = ProjectStorage() );

QString getPath( const FileTracker & ft );

QString getPath( const PathTracker & pt, const ProjectStorage & storage = ProjectStorage() );

QString getFileName( const FileTracker & ft );
