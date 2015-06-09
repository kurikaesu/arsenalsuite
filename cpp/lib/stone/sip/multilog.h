
class Multilog : QObject
{
Q_OBJECT
public:

	Multilog( const QString & logfile, bool stdout_option = false, int severity = 1, int maxfiles = 10, unsigned int maxsize = 2097152);
	
	~Multilog();

	/** Logs a message to the logfile.  The message will only be logged
	 *  if the severity is <= loglevel, and if the file matches the file
	 *  filter, if the filter is set. */
	void log( const int severity, const QString & logmessage, const QString & file = QString(), const QString & function = QString() );

	/** If filesFilter is set to a valid regular expression, then
	 *  only log messages that come from a file that matches the
	 *  expression will be logged */
	QRegExp filesFilter() const;
	void setFilesFilter( const QRegExp & );

/*	QRegExp functionsFilter() const;
	void setFunctionsFilter( const QRegExp & ); */

	/**  Sets whether to echo the log data to stdout */
	void setStdOut( bool stdout_option );
	bool stdOut() const;

	/**  if loglevel of message is less than or equal to desired loglevel, message will be logged
	 */
	void setLogLevel(int severity );
	int logLevel() const;

	/**  \param maxfiles sets the number of log files to be kept in the directory
	 *	 all other logfiles that match the logfile name passed to the ctor will
	 *   be deleted
	 */
	void setMaxFiles( int maxfiles );
	int maxFiles() const;

	/** maxsize is maximum size of each individual logfile before auto-rotation occurs
	 */
	void setMaxSize( unsigned int maxsize );
	int maxSize() const;

	QString logFileName();
signals:
	void logged( const QString & );
};

#endif

