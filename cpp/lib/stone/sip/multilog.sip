
class Multilog : QObject
{
%TypeHeaderCode
#include <multilog.h>
%End
public:

	Multilog( const QString & logfile, bool stdout_option = false, int severity = 1, int maxfiles = 10, unsigned int maxsize = 2097152);
	
	~Multilog();

	void log( const int severity, const QString & logmessage, const QString & file = QString(), const QString & function = QString() );

	QRegExp filesFilter() const;
	void setFilesFilter( const QRegExp & );

/*	QRegExp functionsFilter() const;
	void setFunctionsFilter( const QRegExp & ); */

	void setStdOut( bool stdout_option );
	bool stdOut() const;

	void setLogLevel(int severity );
	int logLevel() const;

	void setMaxFiles( int maxfiles );
	int maxFiles() const;

	void setMaxSize( unsigned int maxsize );
	int maxSize() const;

	QString logFileName();
signals:
	void logged( const QString & );
};


