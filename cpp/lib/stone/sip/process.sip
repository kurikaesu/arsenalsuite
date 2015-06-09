
void openExplorer( const QString & );

void openFrameCycler( const QString & );

void openURL( const QString & );

void vncHost(const QString & host);

// Returns true if there is a process running with pid
// and name( if name is not null )
bool isRunning(int pid, const QString & name = QString::null);

// Kills process with pid
bool killProcess(int pid);

// Returns the pid of this process
int processID();

// Reads a pid from 'filePath'
int pidFromFile( const QString & filePath );

// Writes a pid to 'filePath'
bool pidToFile( const QString & filePath );

// Returns the number of processes that are named 'name'
// Return a list of pids if pidList!=0
int pidsByName( const QString & name, QList<int> * pidList=0, bool caseSensitive=false );

bool killAll( const QString & processName, int timeout = 0, bool caseSensitive=false );

bool findMatchingWindow( int pid, QStringList titles, bool matchProcessChildren, bool caseSensitive, QString * foundTitle /Out/ );

bool setProcessPriorityClass( int pid, unsigned long priorityClass );

int setProcessPriorityClassByName( const QString & name, unsigned long priorityClass );

/// Returns the amount of free memory in the system
struct SystemMemInfo {
	SystemMemInfo();
	enum Caps {
		FreeMemory = 1,
		CachedMemory = 2,
		TotalMemory = 4
	};
	// Kilobytes
	int freeMemory;
	int cachedMemory;
	int totalMemory;
	int caps;
};

SystemMemInfo systemMemoryInfo();

struct ProcessMemInfo {
	ProcessMemInfo();
	enum Caps {
		CurrentSize = 1,
		MaxSize = 2,
		Recursive = 4,
	};
	// Kilobytes
	int currentSize;
	int maxSize;
	int caps;
};

int qprocessId( QProcess * );

ProcessMemInfo processMemoryInfo( int pid, bool recursive = false );

int processParentId( int pid );

QList<int> processChildrenIds( int pid, bool recursive = false );

bool systemShutdown( bool reboot = false, const QString & message = QString() );

%If (WS_WIN)

int killAllWindows( const QString & windowTitleRE );

%MappedType _HWND
{
%TypeHeaderCode
typedef HWND _HWND;
%End
%ConvertFromTypeCode
	PyObject * l;
	l = PyLong_FromVoidPtr((void*)(*sipCpp));
	return l;
%End
%ConvertToTypeCode
	if( sipIsErr == NULL ) {
		if( !PyLong_Check(sipPy) )
			return 0;
		return 1;
	}
	HWND ret = (HWND)PyLong_AsVoidPtr(sipPy);
	*sipCppPtr = new HWND(ret);
    return sipGetState(sipTransferObj);
%End
};

QList<_HWND> getWindowsByName( const QString & nameRE );

bool processHasNamedWindow( int pid, const QString & nameRE, bool processRecursive = false );

qint32 windowProcess( _HWND hWin );

struct WindowInfo {
	QString title;
	int processId;
	_HWND hWin;
};

QList<WindowInfo> windowInfoByProcess( int pid, bool recursive = false );

bool isWow64();

bool disableWindowsErrorReporting( const QString & );

QString localDomain();

bool qSetCurrentProcessExplicitAppUserModelID( const QString & appId );

QString currentExecutableFilePath();

bool saveScreenShot( const QString & path );

%End

Interval systemUpTime();

//Interval idleTime();
