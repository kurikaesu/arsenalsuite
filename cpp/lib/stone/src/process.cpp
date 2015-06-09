
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

#include <qglobal.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qlist.h>
#include <qprocess.h>
#include <qmessagebox.h>
#include <qdatetime.h>
#include <qtextstream.h>

//#include <unistd.h>

#include "process.h"
#include "blurqt.h"
#include "iniconfig.h"
#include "path.h"

#ifndef Q_OS_WIN

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

bool setProcessPriorityClass( int /*pid*/, unsigned long /*priorityClass*/ )
{ return false; }

int setProcessPriorityClassByName( const QString & /*name*/, unsigned long /*priorityClass*/ )
{ return 0; }


bool isRunning(int pid, const QString &)
{
	if( pid <= 0 ) return false;
	int res = kill( pid, 0 );
	if( res == 0 )
		return true;
	else
		return false;
}

// Windows version in process_win.cpp
#ifndef Q_OS_WIN
QDateTime processStartTime(int pid)
{
	QDateTime ret;
	return ret;
}
#endif

int processID()
{
	return getpid();
}

int pidsByName( const QString & name , QList<int> * pidList , bool )
{
	QProcess p;
#ifdef Q_OS_LINUX
	QString cmd = "ps -o pid= -C "+name;
#endif
#ifdef Q_OS_MAC
	QString cmd = "sh -c \"ps axw -o pid,command | grep "+name+ "| cut -f1 -d ' '\"";
#endif
	QString fromPs = backtick(cmd);

	QStringList psLines = QString(fromPs).split("\n");
	if( psLines.size() < 2 )
		return 0;

	int pidCount = 0;
	foreach( QString pidLine, psLines ) {
		//LOG_5( "pidsByName read: "+pidLine );
		pidCount++;
		uint pid = pidLine.toUInt();
		if( pidList && pid > 0 )
			(*pidList) += pid;
	}
#ifdef Q_OS_MAC
	return pidCount-3;
#endif
	return pidCount;
}

bool killProcess(int pid)
{
	return kill( pid, SIGKILL )==0;
}

bool systemShutdown( bool reboot, const QString & )
{
	return QProcess::startDetached("shutdown",QStringList() << (reboot ? "-r" : "-h") << "now");
}

bool findMatchingWindow( int pid, QStringList titles, bool matchProcessChildren, bool caseSensitive, QString * foundTitle  )
{
	return false;
}

#endif // !Q_OS_WIN

bool killAll( const QString & processName, int timeout, bool caseSensitive )
{
	QList<int> pids;
	pidsByName( processName, &pids, caseSensitive );
	foreach( int pid, pids ) {
		LOG_3( "Killing Process with PID: " + QString::number( pid ) );
		if( pid > 0 )
			killProcess( pid );
	}
	QTime t;
	t.start();
	while( t.elapsed() < timeout*1000 ) {
		if( pidsByName( processName, 0, caseSensitive ) == 0 )
			return true;
#ifdef Q_OS_WIN
		Sleep( 1000 );
#else
		sleep( 1 );
#endif
		//qApp->processEvents( 500 );
	}
	return( pidsByName( processName, 0, caseSensitive ) == 0 );
}

int pidFromFile( const QString & pidFileName )
{
	return readFullFile(pidFileName).simplified().toInt();
}

bool pidToFile( const QString & fileName )
{
	return writeFullFile( fileName, QString::number( processID() ) );
}

void openExplorer( const QString & path )
{
#ifdef Q_OS_WIN
	if( Stone::Path(path).fileExists() )
		QProcess::startDetached("explorer /e,/select," + QString(path).replace("/","\\") );
	else
		QProcess::startDetached("explorer /e," + QString(path).replace("/","\\") );
#else
	QProcess::startDetached("konqueror", QStringList() << Stone::Path(path).dirPath());
#endif
}

void openURL( const QString & url )
{
#ifdef Q_OS_WIN
	ShellExecute( 0, 0, (WCHAR*)url.utf16(), 0, 0, SW_SHOWNORMAL );
#else
	QStringList args;
	args << "exec" << url;
	QProcess::startDetached( "kfmclient", args );
#endif
}

void openFrameCycler( const QString & path )
{
	IniConfig cfg = userConfig();
	cfg.pushSection( "Assfreezer_Preferences" ); // Should be changed
	QString fcp = cfg.readString( "FrameCyclerPath" );
	QString fca = cfg.readString( "FrameCyclerArgs" );
	cfg.popSection();
	
	#ifdef Q_WS_WIN
	if( fcp.isEmpty() ){
		QStringList fdc;
		fdc += "FrameCycler_Pro_2.70";
		fdc += "FrameCycler_2.7_rc1";
		fdc += "FrameCycler_2.65b4";
		fdc += "FrameCycler_2.65b1";
		fdc += "FrameCycler_2.62";
		for( QStringList::Iterator it = fdc.begin(); it != fdc.end(); ++it ){
			QString versionPath = "C:\\Program Files (x86)\\" + *it + "\\bin\\FrameCycler.exe";
			if( QFile::exists( versionPath ) )
				fcp = versionPath;
		}
		if( fcp.isEmpty() ){
			QMessageBox::warning( 0, "Can't Find FrameCycler", "AssFreezer was unable to detect FrameCycler. "
				"If FrameCycler is installed somewhere other than 'C:\\Program Files\\FrameCycler_...' then you must set the path"
				" in the options dialog.", QMessageBox::Ok, QMessageBox::NoButton );
			return;
		}
	}
	#else
	if( fcp.isEmpty() )
		fcp = "kview";
	#endif // Q_WS_WIN
	QStringList args = fca.split(" ");
	args << path;
	LOG_3("openFrameCycler(): "+ fcp +" "+args.join(" "));
	QProcess::startDetached( fcp, args );
}

void vncHost(const QString & host)
{
#ifdef Q_WS_WIN
	static int tempNum = 1;
	const char * VNC_LINK =
		"[connection]\nhost=%1\nport=5900\npassword=62d0bc9c593df2d5\n[options]\nuse_encoding_0=1\nuse_encoding_1=1\n"
		"use_encoding_2=1\nuse_encoding_3=0\nuse_encoding_4=1\nuse_encoding_5=1\nuse_encoding_6=1\nuse_encoding_7=1\n"
		"use_encoding_8=1\npreferred_encoding=7\nrestricted=0\nviewonly=0\nfullscreen=0\n8bit=0\nshared=1\nswapmouse=0\n"
		"belldeiconify=0\nemulate3=1\nemulate3timeout=100\nemulate3fuzz=4\ndisableclipboard=0\nlocalcursor=1\nscale_den=1\n"
		"scale_num=1\ncursorshape=1\nnoremotecursor=0\ncompresslevel=8\nquality=0\n";
	QString vncFileName("c:\\temp\\tempvnc" + QString::number(tempNum++) + ".vnc");
	if( !writeFullFile( vncFileName, QString( VNC_LINK ).arg( host ) ) ) {
		LOG_1( "Unable to write vnc link file to " + vncFileName );
		return;
	}
	QProcess::startDetached( "cmd /c start " + vncFileName );
#else
	QProcess::startDetached( QString::fromLatin1("krdc"), QStringList(host + ".blur.com:0") );
#endif // Q_WS_WIN
}

QString backtick(const QString & cmd)
{
	QProcess p;
	p.setProcessChannelMode(QProcess::MergedChannels);
	//LOG_3("process::backtick() starting: "+cmd);
	p.start( cmd );
	p.waitForFinished(-1);
	return QString(p.readAllStandardOutput());
}

SystemMemInfo systemMemoryInfo()
{
	SystemMemInfo ret;
#ifdef Q_OS_WIN
	MEMORYSTATUS stat;
	GlobalMemoryStatus (&stat);
	ret.caps = SystemMemInfo::FreeMemory | SystemMemInfo::TotalMemory;
	ret.freeMemory = stat.dwAvailPhys / 1024;
	ret.totalMemory = stat.dwTotalPhys / 1024;
#endif
#ifdef Q_OS_MAC
#endif

#ifdef Q_OS_LINUX
	QFile procFile( "/proc/meminfo" );
	if( procFile.open( QIODevice::ReadOnly ) ) {
		QStringList lines = QTextStream(&procFile).readAll().split("\n");
		QRegExp free( "MemFree:\\s+(\\d+)\\s+kB" ), cached( "Cached:\\s+(\\d+)\\s+kB" ), total( "MemTotal:\\s+(\\d+)\\s+kB" );
		foreach( QString line, lines ) {
			if( free.exactMatch( line ) ) {
				ret.caps |= SystemMemInfo::FreeMemory;
				ret.freeMemory = free.cap(1).toInt();
			}
			if( cached.exactMatch( line ) ) {
				ret.caps |= SystemMemInfo::CachedMemory;
				ret.cachedMemory = cached.cap(1).toInt();
			}
			if( total.exactMatch( line ) ) {
				ret.caps |= SystemMemInfo::TotalMemory;
				ret.totalMemory = total.cap(1).toInt();
			}
		}
	}
#endif
	return ret;
}

qint32 qprocessId( QProcess * process )
{
	return qpidToId(process->pid());
}

qint32 qpidToId( Q_PID qpid )
{
#ifdef Q_OS_WIN
	return qpid ? qpid->dwProcessId : 0;
#else
	return qpid;
#endif
}

#ifndef Q_OS_WIN // Win32 version in process_win.cpp

static int parsePSDump( qint32 wantedPid, QString psDump, int depth=0)
{
	int memory = 0;
	QRegExp rx("(\\d+)\\s+(\\d+)\\s+(\\d+)");

	QStringList lines = psDump.split("\n");
	foreach( QString line, lines ) {
		if( rx.indexIn(line,0) != -1 ) {
			int pid  = rx.cap(1).toInt();
			int ppid = rx.cap(2).toInt();
			int rss  = rx.cap(3).toInt();
			int matchPid = ppid;
			if( depth == 0 )
				matchPid = pid;
			if( matchPid == wantedPid ) {
				//LOG_3("memoryForPid() read: "+line);
				memory += rss;
				memory += parsePSDump(pid, psDump, depth+1);
			}
		}
	}
	return memory;
}

// TODO: Implement recursive option
ProcessMemInfo processMemoryInfo( qint32 pid, bool /*recursive*/ )
{
	QString psCmd = "ps -A -o pid,ppid,rss";
	LOG_3("Running Cmd: "+psCmd);
	ProcessMemInfo ret;
	ret.caps = ProcessMemInfo::CurrentSize;
	ret.currentSize = parsePSDump(pid,backtick(psCmd));
	return ret;
}

qint32 processParentId( qint32 pid )
{
	if( pid == getpid() )
		return getppid();
	QString statFile( "/proc/" + QString::number(pid) + "/stat" );
	if( QFile::exists( statFile ) ) {
		QFile sf( statFile );
		if( sf.open( QIODevice::ReadOnly ) ) {
			qint32 pid;
			QTextStream(&sf) >> pid;
			sf.close();
			return pid;
		}
	}
	LOG_1( "Unable to get process parent id for process: " + QString::number( pid ) );
	return 0;
}

static QList<qint32> findChildrenFromPS( qint32 _pid, QStringList output, bool recursive )
{
	QList<qint32> ret;
	foreach( QString line, output ) {
		QRegExp rx("(\\d+)\\s+(\\d+)");
		if( rx.indexIn(line,0) != -1 ) {
			int pid  = rx.cap(1).toInt();
			int ppid = rx.cap(2).toInt();
			if( ppid == _pid ) {
				ret << pid;
				if( recursive ) ret += findChildrenFromPS( pid, output, recursive );
			}
		}
	}
	return ret;
}

QList<qint32> processChildrenIds( qint32 _pid, bool recursive )
{
	QList<qint32> ret;
	QString psOutput = backtick("ps -A -o pid,ppid");
	return findChildrenFromPS( _pid, psOutput.split("\n"), recursive );
}

Interval systemUpTime()
{
	QRegExp rx("up\\s+(.*)\\s+\\d+\\s+users");
	if( rx.indexIn( backtick("uptime"), 0 ) >= 0 ) {
		QString uptime = rx.cap(1).replace(","," ");
		return Interval::fromString(uptime);
	}
	return Interval();
}

#endif // !Q_OS_WIN

