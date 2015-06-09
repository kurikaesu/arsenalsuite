

QString generatePathFromTemplate( const ProjectStorage & ps = ProjectStorage() );

QString path( const ProjectStorage & ps = ProjectStorage() );
bool setPath( const QString & path );

bool createPath();

static MappedRecord fromPath( const QString & path, bool matchClosest = false );

static MappedRecord fromPath( const ProjectStorage &, const QString & path, bool caseSensitive = false, bool matchClosest = false );
