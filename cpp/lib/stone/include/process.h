
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of libstone.
 *
 * libstone is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libstone is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libstone; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id$
 */

#ifndef LIB_BLUR_QT_PROCESS_H
#define LIB_BLUR_QT_PROCESS_H

#define UNICODE 1
#define _UNICODE 1

#include <qstring.h>
#include <qlist.h>

#include "blurqt.h"
#include "interval.h"

#include <qprocess.h>

#ifdef Q_OS_WIN
#include <windows.h>

class QIODevice;
class QTimer;

class STONE_EXPORT Win32Process : public QObject
{
Q_OBJECT
public:
	Win32Process( QObject * parent = 0 );

	enum LogonFlag {
		LogonWithoutProfile = 0,
		LogonWithProfile = 1,
		LogonWithNetCredentials = 2
	};
 
	void setLogonFlag( LogonFlag flag );

	/// If this is set, the process will be started in it's own login session
	void setLogon( const QString & userName, const QString & password, const QString & domain = QString() );

	bool start( const QString & program, const QStringList & args = QStringList() );

	void setInheritableIOChannels( bool ioc );

	void setWorkingDirectory( const QString & );

	// An empty string list(default) will use the users
	// default environment.  To create a blank environment
	// pass a string list with 1 empty string.
	void setEnvironment( QStringList env );

	QProcess::ProcessError error() const;
	QString errorString() const;
/*
	QIODevice * stdin();
	QIODevice * stderr();
	QIODevice * stdout();
*/
	int exitCode();
	
	bool isRunning();

	QProcess::ProcessState state() const;

	Q_PID pid();
	
	void terminate();

	static Win32Process * create( QObject * parent, const QString & cmd, const QString & userName = QString(), const QString & password = QString(), const QString & domain = QString() );
signals:
	void error ( QProcess::ProcessError error );
	void finished ( int exitCode, QProcess::ExitStatus exitStatus );
	void readyReadStandardError();
	void readyReadStandardOutput();
	void started();
	void stateChanged( QProcess::ProcessState newState );

protected slots:
	void _checkup();

protected:
	void _error(QProcess::ProcessError, const QString &);

	int mLogonFlag;
	QString mCmd, mUserName, mPassword, mDomain, mWorkingDir, mErrorString;
	QStringList mEnv;
	bool mInheritableIOChannels;
	int mExitCode;
	QProcess::ProcessError mError;
	QProcess::ProcessState mState;
	PROCESS_INFORMATION mProcessInfo;
	QTimer * mCheckupTimer;
	HANDLE stdin_in, stdin_out, stdout_in, stdout_out, stderr_in, stderr_out;
//	friend class ProcessJob;
};

#endif // Q_OS_WIN
/*
class STONE_EXPORT ProcessJob : public QObject
{
Q_OBJECT
public:
	ProcessJob( QObject * parent = 0 );

	bool addProcess( Process * process );

	QList<Process*> processes();

	void terminate();

protected:

#ifdef Q_OS_WIN
	HANDLE mWindowsJobHandle;
#endif // Q_OS_WIN
};
*/

STONE_EXPORT void openExplorer( const QString & );

STONE_EXPORT void openFrameCycler( const QString & );

STONE_EXPORT void openURL( const QString & );

STONE_EXPORT void vncHost(const QString & host);

/// Returns true if there is a process running with pid
/// and name( if name is not null )
STONE_EXPORT bool isRunning(int pid, const QString & name = QString::null);

STONE_EXPORT QDateTime processStartTime(int pid);

/// Kills process with pid
STONE_EXPORT bool killProcess(qint32 pid);

/// Returns the pid of this process
STONE_EXPORT int processID();

/// Reads a pid from 'filePath'
STONE_EXPORT int pidFromFile( const QString & filePath );

/// Writes a pid to 'filePath'
STONE_EXPORT bool pidToFile( const QString & filePath );

/// Returns the number of processes that are named 'name'
/// Return a list of pids if pidList!=0
STONE_EXPORT int pidsByName( const QString & name, QList<int> * pidList=0, bool caseSensitive = false );

STONE_EXPORT bool killAll( const QString & processName, int timeout = 0, bool caseSensitive = false );

STONE_EXPORT QString backtick(const QString & cmd);

/// Enums through all the windows, if any of the window titles
/// matches the titles list and belows to pid or one of it's children(if matchProcessChildren is true),
/// then return true and store the window title found in foundTitle, if not null
STONE_EXPORT bool findMatchingWindow( int pid, QStringList titles, bool matchProcessChildren, bool caseSensitive, QString * foundTitle = 0 );

#ifdef Q_OS_WIN

STONE_EXPORT int killAllWindows( const QString & windowTitleRE );

/// returns a list of window handles
/// for every window that has a title that matches
/// the regular expression nameRE
STONE_EXPORT QList<HWND> getWindowsByName( const QString & nameRE );

STONE_EXPORT bool processHasNamedWindow( int pid, const QString & nameRE, bool processRecursive = false );

#endif // Q_OS_WIN
STONE_EXPORT bool setProcessPriorityClass( int pid, unsigned long priorityClass );

STONE_EXPORT int setProcessPriorityClassByName( const QString & name, unsigned long priorityClass );

/// Returns the amount of free memory in the system
struct STONE_EXPORT SystemMemInfo {
	SystemMemInfo() : freeMemory(0), cachedMemory(0), totalMemory(0), caps(0) {}
	enum Caps {
		FreeMemory = 1,
		CachedMemory = 2,
		TotalMemory = 4
	};
	// Kilobytes
	int freeMemory, cachedMemory, totalMemory;
	int caps;
};

STONE_EXPORT SystemMemInfo systemMemoryInfo();

struct STONE_EXPORT ProcessMemInfo {
	ProcessMemInfo() : currentSize(0), maxSize(0), caps(0) {}
	enum Caps {
		CurrentSize = 1,
		MaxSize = 2,
		Recursive = 4
	};
	// Kilobytes
	int currentSize, maxSize;
	int caps;
};

STONE_EXPORT ProcessMemInfo processMemoryInfo( qint32 pid, bool recursive = false );

class QProcess;
/// Returns the pid of the process
/// Q_PID doesn't return the pid on windows, instead returns
/// a process information struct.  This function works around
/// that so you can get a valid integer pid for all platforms
STONE_EXPORT qint32 qprocessId( QProcess * );
STONE_EXPORT qint32 qpidToId( Q_PID qpid );

STONE_EXPORT qint32 processParentId( qint32 pid );

/// Returns the ids of the children of `pid`
/// If recursive==true, then return the entire process tree
/// including `pid` and all it's descendents
STONE_EXPORT QList<qint32> processChildrenIds( qint32 pid, bool recursive = false );

STONE_EXPORT bool systemShutdown( bool reboot = false, const QString & message = QString() );

#ifdef Q_OS_WIN
/// Win32 only
/// Returns the Process ID of the process that created the window hWin. */
STONE_EXPORT qint32 windowProcess( HWND hWin );

struct STONE_EXPORT WindowInfo {
	QString title;
	qint32 processId;
	HWND hWin;
};
/**
 *  Win32 only
 *  Returns WindowInfo structures filled with the information about all toplevel windows
 *  belonging to the process with pid.  If recursive is true, returns all windows for all
 *  descendant processes along with pid. */
STONE_EXPORT QList<WindowInfo> windowInfoByProcess( qint32 pid, bool recursive = false );

/**
 *	Win32 only
 *	Returns true if running inside  a 32 bit application on 64 bit windows.
 *  Wow64 is the name of window's 32 bit compatibility layer. */
STONE_EXPORT bool isWow64();

/**
 *  Win32 only
 *  Turns off windows error reporting crash dialog for executableName.
 *
 *  Addes executableName key HKEY_LOCAL_MACHINE/Software/Microsoft/PCHealth/ErrorReporting/ExclusionList with
 *  DWORD value of 1.  Sets KEY_WOW64_64KEY when running under Wow64 to workaround registry redirect.
 
 *  If executableName is empty, it will disable reporting and warning dialogs for all apps
 *  by setting
	[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\PCHealth\ErrorReporting]
	"DoReport"=dword:00000000                                                                                                                                                                                                                                                     
	"ShowUI"=dword:00000000                                                                                                                                                                                                                                                       
	[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\Windows Error Reporting]                                                                                                                                                                                                       
	"DontShowUI"=dword:00000001                                                                                                                                                                                                                                                   
 */
STONE_EXPORT bool disableWindowsErrorReporting( const QString & executableName = QString() );

/**
 *  Win32 only
 *  Returns the domain the local computer belongs to.
 */
STONE_EXPORT QString localDomain();

// Qt wrapper for windows 7 SetCurrentProcessExplicitAppUserModelID
// Use this to provide proper task bar grouping
STONE_EXPORT bool qSetCurrentProcessExplicitAppUserModelID( const QString & appId );

STONE_EXPORT QString currentExecutableFilePath();

/// Path must be a valid file path with an image extension that can be saved by QImage
STONE_EXPORT bool saveScreenShot( const QString & path );

#endif

STONE_EXPORT Interval systemUpTime();

/// Interval since the last input event(mouse movement, keyboard click, etc)
//STONE_EXPORT Interval idleTime();

#endif // LIB_BLUR_QT_PROCESS_H

