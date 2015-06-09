
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

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

#include <qfile.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <qstringlist.h>
#include <qregexp.h>

#include "blurqt.h"
#include "database.h"

#ifdef Q_OS_WIN

#define UNICODE 1
#define _UNICODE 1

#include "windows.h"
#include <tchar.h>
#include "accctrl.h"
#include "aclapi.h"
#else
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "path.h"

QString makeFramePath( const QString & bp, uint in, uint padWidth, bool endDigitsAreFrameNumber )
{
	QString num;
	num = num.sprintf(QString("%0" + QString::number(padWidth) + "i").toLatin1().constData(), in);
	QString path = bp;
	int loc = path.lastIndexOf('.');
	if( loc==-1 )
		return QString::null;
	QString tail = path.mid( loc );
	if( endDigitsAreFrameNumber ) {
		int remNums=0;
		while( loc > 0 && remNums < num.size() && QChar(path[loc-1]).isNumber() ){
			remNums++;
			loc--;
		}
		if( loc==0 )
			return QString::null;
	}
	QString head = path.left( loc );
	path = head + num + tail;
	path.replace('\\', "/");
	return path;
}

QStringList filesFromFramePath( const QString & basepath, QList<int> * frames, int * padWidth )
{
	Path p(basepath);
	QFileInfo fi(p.fileName());
	QString baseName = fi.completeBaseName();
	baseName = baseName.remove( QRegExp( "\\d+$" ) );
	QStringList possibleMatches = QDir( p.dirPath() ).entryList(QStringList() << (baseName + "*" + fi.suffix()), QDir::Files);
	QRegExp fileMatch( baseName + "(\\d+)\\." + fi.suffix() );
	QStringList ret;
	if( padWidth )
		*padWidth = 4;
	foreach( QString f, possibleMatches ) {
		if( fileMatch.exactMatch( f ) ) {
			if( frames ) {
				int frame = fileMatch.cap(1).toInt();
				(*frames) += frame;
				if( padWidth && *padWidth > fileMatch.cap(1).size() )
					*padWidth = fileMatch.cap(1).size();
			}
			ret << f;
		}
	}
	return ret;
}

QString framePathBaseName( const QString & framePath, int * frame )
{
	QFileInfo fi(framePath);
	QString baseName = fi.completeBaseName();
	QRegExp frameNumberMatch( "(\\d+)$" );
	baseName = baseName.remove( frameNumberMatch );
	if( frame )
		*frame = frameNumberMatch.cap(1).toInt();
	return baseName + "." + fi.suffix();
}

QList<int> expandNumberList( const QString & list, bool * valid )
{
	QList<int> ret;
	QStringList ranges = list.split(',');

	for( QStringList::Iterator it = ranges.begin(); it != ranges.end(); ++it ) {
		// Each portion of the frame list can be in the following forms
		// Wonder if anyone actually uses negatives frame numbers???
		// 1 			-- Single positive
		// -1			-- Single negative
		// 1-2			-- Double, both positive
		// -1-2			-- Double, first negative
		// -2--1		-- Double, both negative
		bool sn = false, en = false, is_range=false;
		QString ss, es, range( (*it).simplified() );
		int pos=0;

		if( range[pos] == '-' ) {
			sn = true;
			pos++;
		}

		for( ; pos<(int)range.length(); pos++ ) {
			if( range[pos] == '-' ) {
				is_range = true;
				pos++;
				break;
			}

			if( !range[pos].isNumber() )
				goto OUT_ERROR;
				
			ss += range[pos];
		}
		
		if( is_range && range[pos] == '-' ) {
			en = true;
			pos++;
		}
		
		for( ; pos<(int)range.length(); pos++ ) {
			if( !range[pos].isNumber() )
				goto OUT_ERROR;
			es += range[pos];
		}

		bool valid;
		int start = ss.toInt( &valid );
		if( !valid )
			goto OUT_ERROR;
		if ( sn )
			start = -start;

		if( is_range ) {
			int end = es.toInt( &valid );
			if( !valid )
				goto OUT_ERROR;
			if( en )
				end = -end;
			if( end < start )
				goto OUT_ERROR;
			for( pos=start; pos<=end; pos++ )
				ret += pos;
		} else
			ret += start;
	}

	if( valid )
		*valid = true;

	return ret;
OUT_ERROR:
	
	if( valid )
		*valid = false;
	return ret;
}

static inline QString listOrSingle( int start, int end )
{
	if( end != INT_MAX && start != end )
		return QString( "%1-%2" ).arg(start).arg(end);
	return QString::number(start);
}

QString compactNumberList( const QList<int> & _list )
{
	int first = INT_MAX, previous = INT_MAX;
	QStringList retList;
	QList<int> list(_list);
	qSort(list);
	foreach( int n, list ) {
		if( first == INT_MAX )
			first = n;
		if( previous != INT_MAX && previous + 1 < n ) {
			retList.append( listOrSingle(first,previous) );
			previous = INT_MAX;
			first = n;
		}
		previous = n;
	}
	if( first != INT_MAX )
		retList.append( listOrSingle(first,previous) );
	return retList.join(",");
}

QString readFullFile( const QString & path, bool * error )
{
	QFile file(path);
	if( !file.open( QIODevice::ReadOnly ) ) {
		if( error ) *error = true;
		return QString();
	}
	return QTextStream(&file).readAll();
}

bool writeFullFile( const QString & path, const QString & contents )
{
	QFile file(path);
	if( !file.open( QIODevice::WriteOnly ) )
		return false;
	QTextStream(&file) << contents;
	return true;
}

QString pathOwner( const QString & path, QString * errorMessage )
{
#ifdef Q_OS_WIN
	QPair<QString,QString> pair = pathOwnerDomain( path, errorMessage );
	return pair.first;
#else
	struct stat sb;
	int statRet = stat( path.toLatin1(), &sb );
	if( statRet == -1 ) {
		if( errorMessage ) *errorMessage = "Stat failed";
		return QString();
	}
	
	struct passwd * pas = getpwuid(sb.st_uid);
	if( !pas ) {
		if( errorMessage ) *errorMessage = "getpwuid failed";
		return QString();
	}
	
	return QString::fromLatin1( pas->pw_name );
#endif
}

#ifdef Q_OS_WIN

static QString hostFromUnc( const QString & path )
{
	if( path.startsWith( "\\\\" ) ) {
		int firstHostChar = 2;
		while( firstHostChar + 1 < path.size() && path[firstHostChar] == '\\' ) firstHostChar++;
		int lastHostChar = qMin(firstHostChar + 1, path.size());
		while( lastHostChar + 1 < path.size() && path[lastHostChar] != '\\' ) lastHostChar++;
		return path.mid(firstHostChar,lastHostChar - firstHostChar);
	}
	return QString();
}

QPair<QString,QString> pathOwnerDomain( const QString & _path, QString * errorMessage )
{
	DWORD dwRtnCode = 0;
	PSID pSidOwner = NULL;
	BOOL bRtnBool = TRUE;
	LPTSTR AcctName = 0, DomainName = 0;
	DWORD dwAcctName = 1, dwDomainName = 1;
	SID_NAME_USE eUse = SidTypeUnknown;
	HANDLE hFile;
	PSECURITY_DESCRIPTOR pSD = NULL;
	QString host;
	LPTSTR fileHost = 0;

	QString path(_path);
	path.replace( "/", "\\" );
	
	// Get the handle of the file or path
	hFile = CreateFile( (WCHAR*)path.utf16(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

	// Check GetLastError for CreateFile error code.
	if (hFile == INVALID_HANDLE_VALUE) {
		if( errorMessage ) *errorMessage = QString( "Error opening file to get security info: %1" ).arg( GetLastError() );
		return qMakePair(QString(),QString());
	}

	// Get the owner SID of the file.
	dwRtnCode = GetSecurityInfo( hFile, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION, &pSidOwner, NULL, NULL, NULL, &pSD);

	// Check GetLastError for GetSecurityInfo error condition.
	if (dwRtnCode != ERROR_SUCCESS) {
		if( errorMessage ) *errorMessage = QString( "GetSecurityInfo failed: %1" ).arg( GetLastError() );
		CloseHandle(hFile);
		return qMakePair(QString(),QString());
	}

	if( path.startsWith( "\\\\" ) )
		host = hostFromUnc( path );
	else
		host = hostFromUnc( driveMapping( path[0].toLatin1() ) );
	
	if( !host.isEmpty() )
		fileHost = (LPTSTR)host.utf16();

	// First call to LookupAccountSid to get the buffer sizes.
	bRtnBool = LookupAccountSid( fileHost /* local computer */, pSidOwner, AcctName, (LPDWORD)&dwAcctName, DomainName, (LPDWORD)&dwDomainName, &eUse);

	// Allocate memory for the buffers.
	AcctName = (LPTSTR)GlobalAlloc( GMEM_FIXED, dwAcctName * sizeof(TCHAR));
	if( dwDomainName )
		DomainName = (LPTSTR)GlobalAlloc( GMEM_FIXED, dwDomainName * sizeof(TCHAR) );

	// Check GetLastError for GlobalAlloc error condition.
	if (AcctName == NULL || (dwDomainName && !DomainName)) {
		if( errorMessage ) *errorMessage = QString( "Failed to allocate space for LookupAccountSid: %1" ).arg( GetLastError() );
		GlobalFree((HGLOBAL)AcctName);
		GlobalFree((HGLOBAL)DomainName);
		CloseHandle(hFile);
		LocalFree(pSD);
		return qMakePair(QString(),QString());
	}

	// Second call to LookupAccountSid to get the account name.
	bRtnBool = LookupAccountSid( fileHost /* local computer */, pSidOwner, AcctName, (LPDWORD)&dwAcctName, DomainName, (LPDWORD)&dwDomainName, &eUse);

	// Check GetLastError for LookupAccountSid error condition.
	if (bRtnBool == FALSE) {
		if( errorMessage ) {
			DWORD dwErrorCode = GetLastError();
			if (dwErrorCode == ERROR_NONE_MAPPED)
				*errorMessage = QString( "Account owner not found for specified SID." );
			else 
				*errorMessage = QString( "Failed to allocate space for LookupAccountSid: %1" ).arg( dwErrorCode );
		}
		GlobalFree((HGLOBAL)AcctName);
		GlobalFree((HGLOBAL)DomainName);
		CloseHandle(hFile);
		LocalFree(pSD);
		return qMakePair(QString(),QString());
	}

	QPair<QString,QString> ret = qMakePair( QString::fromWCharArray( AcctName ), QString::fromWCharArray( DomainName ) );
	      
	GlobalFree((HGLOBAL)AcctName);
	GlobalFree((HGLOBAL)DomainName);
	CloseHandle(hFile);
	LocalFree(pSD);

	return ret;
}

#endif // Q_OS_WIN


namespace Stone {

Path::Path( const QString & path )
: mPath( winPath( path ) )
{
	if( dirExists() && mPath.right(1) != "/" )
		mPath += "/";
}

Path Path::operator+( const Path & other ) const
{
	return Path( mPath + other.mPath );
}

Path & Path::operator+=( const Path & other )
{
	mPath = osPath( mPath + other.mPath );
	return *this;
}

Path Path::operator+( const QString & add ) const
{
	return Path( mPath + add );
}

Path & Path::operator+=( const QString & add )
{
	mPath = osPath( mPath + add );
	return *this;
}

QString Path::path() const
{
	return mPath;
}

bool Path::isAbsolute() const
{
	return !isRelative();
}

bool Path::isRelative() const
{
	return mPath[0].isLetter() && mPath[1] != ':';
}

Path Path::chopLevel( int level ) const
{
	if( level == 0 )
		return Path( drive() );

	QStringList ret;
	QStringList parts = mPath.split('/');
	for( int i=0; i<=level && i<parts.size(); i++ ) {
		QString cur = parts[i];
		ret += parts[i];
		if( i == 0 && cur[1] != ':' )
			i++;
	}
	return Path( ret.join( "/" ) );
}

int Path::level() const
{
	int parts = mPath.count( '/' );
	if( !mPath.isEmpty() && mPath[mPath.size()-1] == '/' )
		parts--;
	return parts;
}

QString Path::operator[](int level)
{
	QStringList ret;
	QStringList parts = mPath.split('/');
	if( level < 0 )
		level = level + parts.size();
	if( level >= 0 && level < parts.size() )
		return parts[level];
	return QString();
}

QString Path::drive() const
{
	if( mPath[0].isLetter() && mPath[1] == ':' )
		return QString(mPath[0].toLower()) + ":";
	if( mPath[0] == '/' )
		return "/";
	return QString();
}

QString Path::stripDrive() const
{
	QString d = drive();
	if( !d.isEmpty() )
		return mPath.mid(d.length());
	return mPath;
}

QString Path::fileName() const
{
	return mPath.section("/",-1);
}
	
QString Path::dirName() const
{
	return mPath.section("/",-2,-2);
}

QString Path::dirPath() const
{
	return mPath.section("/", 0, -2) + "/";
}
	
QString Path::dbDirPath() const
{
	return winPath( dirPath() );
}

QString Path::dbPath() const
{
	return winPath( mPath );
}

Path Path::dir() const
{
	return Path( dirPath() );
}

Path Path::parent() const
{
	QString dp( dirPath() );
	if( dp != mPath )
		return dp;
	return dp.section("/", 0, -3) + "/";
}

bool Path::fileExists() const
{
	return QFileInfo( mPath ).isFile();
}

bool Path::dirExists() const
{
	return QFileInfo( mPath ).isDir();
}

bool Path::symLinkExists() const
{
	return QFileInfo( mPath ).isSymLink();
}

bool Path::exists() const
{
	QFileInfo info( mPath );
	return (info.isFile() || info.isDir() || info.isSymLink());
}

bool Path::mkdir( int makeParents  )
{
	return Path::mkdir( dirPath(), makeParents );
}

bool Path::move( const Path & dest ) const
{
	return QDir().rename( mPath, dest.path() );
}

bool Path::copy( const Path & dest ) const
{
	return copy( mPath, dest.path() );
}

static bool recursiveRemove( const QString & path, QString * error )
{
	if( !QFileInfo( path ).isDir() )
		return true;
	LOG_5( "Path::remove: Starting recursive removal of directory: " + path );
	QDir dir(path);
	QFileInfoList list = dir.entryInfoList( QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot );
	foreach( QFileInfo fi, list ) {
		if( fi.isDir() ) {
			bool res = recursiveRemove( fi.filePath(), error );
			if( !res ) return false;
		} else {
			bool res = QFile::remove( fi.filePath() );
			LOG_5( "Path::remove: Removing file " + fi.filePath() );
			if( !res ) {
				if( error )
					*error = "QFile::remove failed to remove file: " + fi.filePath();
				return false;
			}
		}
	}
	LOG_5( "Path::remove: Removing directory: " + path );
	return dir.rmdir(path);
}

bool Path::remove( bool dirRecursive, QString * error )
{
	if( dirExists() ) {
		if( !dirRecursive ) {
			LOG_5( "Path::remove: Called with a directory: " + mPath + " with dirRecursive=false" );
			return false;
		}
		return recursiveRemove( mPath, error );
	} else if( fileExists() ) {
		bool res = QFile::remove( mPath );
		if( !res && error )
			*error = "QFile::remove failed to remove file: " + mPath;
		return res;
	} else
		LOG_5( "Path::remove: Couldn't find " + mPath + " to remove" );
	return true;
}

bool Path::remove( const QString & path, bool dirRecursive, QString * error )
{
	return Path( path ).remove( dirRecursive, error );
}

bool Path::copy( const QString & src, const QString & dest )
{
	Path srcp( src ), destp( dest );
	if( !srcp.fileExists() || !destp.dir().dirExists() )
		return false;
#ifdef Q_OS_WIN
	bool s_unc = false, d_unc = false;
	
	QString srcw( srcp.path() );
	srcw = srcw.replace( "/", "\\" );
	QString destw( destp.path() );
	destw = destw.replace( "/", "\\" );
	
	if( srcw[0] == '\\' && srcw[1] == '\\' ) s_unc = true;
	if( destw[0] == '\\' && destw[1] == '\\' ) d_unc = true;
	srcw.replace( "\\\\", "\\" );
	if( s_unc ) srcw = "\\" + srcw;
	destw.replace( "\\\\", "\\" );
	if( d_unc ) destw = "\\" + destw;
	
	bool ret = CopyFileA( srcw.toLatin1(), destw.toLatin1(), false ) != 0;
	if( !ret ) {
		DWORD err = GetLastError();
		LOG_1( "Copy failed with error: " + QString::number(err) );
		LOG_1( "Src: " + srcw +  "Dest: " + destw );
	}
	return ret;
#else
	system( QString( "cp " + srcp.path() + " " + destp.path() ).toLatin1() );
	return true;
#endif
}

bool Path::mkdir( const QString & path, int makeParents )
{
	QString parent( Path( path ).parent().dirPath() );
	if( !QDir().exists( parent ) )
	{
		if( makeParents < 1 || !Path::mkdir( parent, makeParents-1 ) )
			return false;
	}
	if( QDir().exists( path ) || QDir().mkdir( path ) )
		return true;
	return false;
}

bool Path::move( const QString & src, const QString & dest )
{
	return QDir().rename( src, dest );
}

bool Path::exists( const QString & path )
{
	return Path( path ).exists();
}

static QStringList sUnixDriveMappings;
static bool sAddedDriveMappings = false;

static void addUnixDriveMappings()
{
	// Already got them
	if( sAddedDriveMappings )
		return;

	Database * db = Database::current();
	if( !db ) return;

	Table * t = Database::current()->tableByName( "config" );
	if( !t ) {
//		qWarning( "path.cpp addUnixDriveMappings: Couldn't find Config table" );
		return;
	}
	
	Index * name = 0;
	IndexList il = t->indexes();
	foreach( Index * idx, il )
		if( idx->schema()->name() == "Name" )
			name = idx;
	
	if( !name ) {
//		qWarning( "path.cpp addUnixDriveMappings: Couldn't find Name index" );
		return;
	}
		
	Record r = name->recordByIndex( "driveToUnixPath" );
	if( !r.isRecord() ) {
//		qWarning( "path.cpp addUnixDriveMappings: Couldn't find driveToUnixPath record" );
		return;
	}
		
	sUnixDriveMappings = r.getValue( "value" ).toString().split(',');
	sAddedDriveMappings = true;
	return;

}

QString Path::winPath( const QString & path )
{
	QString ret( path );
	ret.replace( "\\", "/" );
	bool isUnc = ret.left(2) == "//";
	ret.replace( "//", "/" );
	if ( isUnc ) ret = "/" + ret;
	//addUnixDriveMappings();
	if( ret[0] == '/' ){
		for( QStringList::Iterator it = sUnixDriveMappings.begin(); it != sUnixDriveMappings.end(); ++it ){
			QString unx = (*it).mid(2);
			if( ret.left( unx.length() ) == unx )
				ret.replace( unx, (*it).left( 2 ) );
		}
	}
	return ret;
}

QString Path::unixPath( const QString & path )
{
	QString ret( path );
	ret.replace( "\\", "/" );
	ret.replace( "//", "/" );
	//addUnixDriveMappings();
	if( ret[1] == ':' ){
		QStringList matches = sUnixDriveMappings.filter( ret.left( 2 ) );
		if( matches.size() == 1 )
			ret = matches[0].mid(2) + ret.mid(2);
	}
	return ret;
}

QString Path::osPath( const QString & path )
{
#ifdef Q_OS_WIN
	return winPath( path );
#else
	return unixPath( path );
#endif
}

} //namespace

#ifdef Q_OS_WIN

QString driveMapping( char driveLetter )
{
	QString drive, ret;
	drive += driveLetter;
	drive += ':';

	WCHAR lpRemoteName[500];
	DWORD length = 500;

	DWORD dwResult = WNetGetConnection((WCHAR*)drive.utf16(),lpRemoteName,&length);

	if( dwResult == NO_ERROR )
		ret = QString::fromUtf16( (const short unsigned int *)lpRemoteName );

	return ret;
}

QString getWindowsErrorMessage( DWORD id )
{
	WCHAR * buffer;
	QString ret;

	int len = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, 0, id, 0, (WCHAR*)&buffer, 0, 0 );
	
	if( len )
		ret = QString::fromUtf16( (const short unsigned int*)buffer );

	if( buffer )
		LocalFree( buffer );

	return ret;
}

// NetUseAdd
// WNetCancelConnection2
// WNetGetConnection
bool mapDrive( char driveLetter, const QString & uncPath, bool forceUnmount, QString * errMsg )
{
	QString currentMapping = driveMapping( driveLetter );
	if( currentMapping == uncPath ) {
		LOG_5( "Drive " + QString(driveLetter) + ": is already mapped to " + uncPath );
		return true;
	}

	QString drive;
	drive += driveLetter;
	drive += ':';

	DWORD dwResult = WNetCancelConnection2( (TCHAR*)drive.utf16(), CONNECT_UPDATE_PROFILE, forceUnmount );

	if( dwResult != NO_ERROR && dwResult != ERROR_NOT_CONNECTED ) {
		if( errMsg )
			*errMsg = "Unable to unmap current mapping: " + currentMapping + " error was: " + getWindowsErrorMessage(dwResult);
		return false;
	}

	NETRESOURCE nr; 
	nr.dwScope = RESOURCE_CONNECTED;
	nr.dwType = RESOURCETYPE_DISK;
	nr.dwDisplayType = RESOURCEDISPLAYTYPE_SHARE;
	nr.dwUsage = RESOURCEUSAGE_CONNECTABLE;
	nr.lpLocalName = (TCHAR*)drive.utf16();
	nr.lpRemoteName = (TCHAR*)uncPath.utf16();
	nr.lpComment = 0;
	nr.lpProvider = 0;

	// Call the WNetAddConnection2 function to make the connection,
	dwResult = WNetAddConnection2(&nr,
		0,                  // no password 
		0,                  // logged-in user 
		0);
	
	// Process errors.
	//  The local device is already connected to a network resource.
	//
	if( dwResult != NO_ERROR && errMsg )
		*errMsg = getWindowsErrorMessage( dwResult );

	return dwResult == NO_ERROR;
}

/*
QValueList< QPair<char,QString> > driveMappings()
{
	QValueList< QPair<char,QString> > ret;

	BOOL WINAPI EnumerateFunc(HWND hwnd, 
							HDC hdc, 
							LPNETRESOURCE lpnr)
	{

	DWORD dwResult, dwResultEnum;
	HANDLE hEnum;
	DWORD cbBuffer = 16384;      // 16K is a good size
	DWORD cEntries = -1;         // enumerate all possible entries
	LPNETRESOURCE lpnrLocal;     // pointer to enumerated structures
	DWORD i;
	//
	// Call the WNetOpenEnum function to begin the enumeration.
	//
	dwResult = WNetOpenEnum(RESOURCE_GLOBALNET, // all network resources
							RESOURCETYPE_ANY,   // all resources
							0,        // enumerate all resources
							lpnr,     // NULL first time the function is called
							&hEnum);  // handle to the resource
	
	if (dwResult != NO_ERROR)
		return ret;
	//
	// Call the GlobalAlloc function to allocate resources.
	//
	lpnrLocal = (LPNETRESOURCE) GlobalAlloc(GPTR, cbBuffer);
	if (lpnrLocal == NULL)
		return ret;
	
	do
	{
		//
		// Initialize the buffer.
		//
		ZeroMemory(lpnrLocal, cbBuffer);
		//
		// Call the WNetEnumResource function to continue
		//  the enumeration.
		//
		dwResultEnum = WNetEnumResource(hEnum,      // resource handle
										&cEntries,  // defined locally as -1
										lpnrLocal,  // LPNETRESOURCE
										&cbBuffer); // buffer size
		//
		// If the call succeeds, loop through the structures.
		//
		if (dwResultEnum == NO_ERROR)
		{
			for(i = 0; i < cEntries; i++)
			{
				// Call an application-defined function to
				//  display the contents of the NETRESOURCE structures.
				//
				DisplayStruct(hdc, &lpnrLocal[i]);
		
				// If the NETRESOURCE structure represents a container resource, 
				//  call the EnumerateFunc function recursively.
		
				if(RESOURCEUSAGE_CONTAINER == (lpnrLocal[i].dwUsage
											& RESOURCEUSAGE_CONTAINER))
				if(!EnumerateFunc(hwnd, hdc, &lpnrLocal[i]))
					TextOut(hdc, 10, 10, "EnumerateFunc returned FALSE.", 29);
			}
		}
		// Process errors.
		//
		else if (dwResultEnum != ERROR_NO_MORE_ITEMS)
		{
			NetErrorHandler(hwnd, dwResultEnum, (LPSTR)"WNetEnumResource");
			break;
		}
	}
	//
	// End do.
	//
	while(dwResultEnum != ERROR_NO_MORE_ITEMS);

	//
	// Call the GlobalFree function to free the memory.
	//
	GlobalFree((HGLOBAL)lpnrLocal);

	//
	// Call WNetCloseEnum to end the enumeration.
	//
	dwResult = WNetCloseEnum(hEnum);
	
	return ret;
}
*/

#else

QString driveMapping( char  )
{
	return QString();
}

bool mapDrive( char, const QString &, bool, QString * )
{
	return false;
}

#endif // Q_OS_WIN

namespace Stone {

bool Path::checkFileFree( const QString & fileName )
{
#ifdef Q_OS_WIN
	QString winName(fileName);
	winName = winName.replace("/", "\\");
	HANDLE hFile = INVALID_HANDLE_VALUE;
	hFile = CreateFile((TCHAR*)winName.utf16(),    // file to open
		GENERIC_READ,          // open for reading
		FILE_SHARE_READ | FILE_SHARE_WRITE,       // share for reading
		NULL,                  // default security
		OPEN_EXISTING,         // existing file only
		FILE_ATTRIBUTE_NORMAL, // normal file
		NULL);                 // no attr. template
	
 	if( hFile == INVALID_HANDLE_VALUE )
		return false;
		
	FlushFileBuffers( hFile );
	CloseHandle( hFile );
#else
	Q_UNUSED(fileName);
#endif // Q_OS_WIN
	return true;
}

long long Path::dirSize( const QString & path )
{
	long long ret = 0;
	QDir dir(path);
	QFileInfoList fil = dir.entryInfoList( QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot );
	foreach( QFileInfo fi, fil ) {
		LOG_5( "Path::dirSize: Calculating size of: " + fi.filePath() );
		if( fi.isDir() )
			ret += dirSize( fi.filePath() );
		else if( fi.isFile() )
			ret += fi.size();
	}
	LOG_5( "Path::dirSize: Directory: " + path + " contains " + QString::number( ret ) + " bytes" );
	return ret;
}

} // namespace
