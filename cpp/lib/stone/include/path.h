
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

#ifndef PATH_H
#define PATH_H

#include <qglobal.h>
#include <qstring.h>
#include <qlist.h>

#include "blurqt.h"


/// \brief Contructs a filepath by inserting frameNumber before the extension of base path.
/// \ingroup Stone
/// If endDigitsAreFrameNumber == false, then any digits before the extension are left as is, and the frame number is appended,
/// otherwise any end digits are replaced with the frame number
STONE_EXPORT QString makeFramePath( const QString & base, uint frameNumber, uint padWidth = 4, bool endDigitsAreFrameNumber = true );

/// \brief Returns just the filename portion of the path, with the frame number removed
/// \ingroup Stone
STONE_EXPORT QString framePathBaseName( const QString & framePath, int * frameNumber = 0 );

/// \brief Returns all images in the path that match the base filename
/// \ingroup Stone
STONE_EXPORT QStringList filesFromFramePath( const QString & basepath, QList<int> * frames = 0, int * padWidth = 0 );

/// \ingroup Stone
STONE_EXPORT QList<int> expandNumberList( const QString & list, bool * valid = 0 );

/// \ingroup Stone
STONE_EXPORT QString compactNumberList( const QList<int> & list );

/// \ingroup Stone
STONE_EXPORT QString driveMapping( char driveLetter );

/// \ingroup Stone
STONE_EXPORT bool mapDrive( char driveLetter, const QString & uncPath, bool forceUnmap = false, QString * errMsg=0 );
//QValueList< QPair<char,QString> > driveMappings();

STONE_EXPORT QString readFullFile( const QString & path, bool * error = 0 );
STONE_EXPORT bool writeFullFile( const QString & path, const QString & contents );

/// Returns the username of the owner of the path(file or directory)
STONE_EXPORT QString pathOwner( const QString & path, QString * errorMessage );

#ifdef Q_OS_WIN
/// Returns the <username,domain> of the owner of the path(file or directory)
STONE_EXPORT QPair<QString,QString> pathOwnerDomain( const QString & path, QString * errorMessage );

#endif // Q_OS_WIN
/**
 *  \ingroup Stone
 * @{
 */

namespace Stone {

class STONE_EXPORT Path
{
public:
	Path(){}
	Path( const QString & path );
	
	Path operator+( const Path & ) const;
	Path & operator+=( const Path & );
	
	Path operator+( const QString & ) const;
	Path & operator+=( const QString & );
	
	/// Returns true if this path is a relative path
	bool isRelative() const;

	/// Returns true if this path is an absolute path
	bool isAbsolute() const;

	/** Each directory is a level, with level 0 just / or the drive letter 'g:/'
	 * level 1 would be /a_directory/ or /a_file or g:/a_dir/ or g:/a_file
	 * etc.. */
	Path chopLevel( int level ) const;
	
	/** Returns the level of this path
	 * '/' or 'C:/' is 0
	 * '/test', '/test/', 'C:/test', 'C:/test/' are 1
	 * '/test/test', etc is 2
	 */
	int level() const;

	/** Returns the section of the path at level
	 *  Example:
	 *  p = Path('G:/this/is/a/file')
	 *  p[0] == 'G:'
	 *  p[1] == 'this'...etc.
	 *  If level is negative, lookup is from right to
	 * left, -1 is the right most section
	 *  p[-1] == 'file'
	 *  p[-2] == 'a'
	 */
	QString operator[](int level);

	/** Returns either the drive letter 'c:','g:', or '/' for a
	 * path without a drive letter, or an empty string for
	 * a relative path
	 */
	QString drive() const;

	/// Returns the path with the drive letter and colon removed
	QString stripDrive() const;

	/** Returns the full path stored by this object
	 *
	 * Path( "/home/user/file" ).path() returns "/home/user/file"
	 * returns the path in the current os form
	 */
	QString path() const;
	
	/** Returns the filename portion of the path
	 *
	 * Path( "/home/user/file" ).fileName() returns "file"
	 * Path( "/home/user/" ).fileName() returns ""
	 * Path( "/home/user" ).fileName() returns "user"
	 */
	QString fileName() const;
	
	/** Returns the name of the directory this path points to
	 * Or the name of the directory that the file resides in that 
	 * this path points to
	 *
	 * Path( "/home/user/file" ).dirName() returns "user"
	 * Path( "/home/user/" ).dirName() returns "user"
	 * Path( "/home/user" ).dirName() returns "home"
	 */
	QString dirName() const;
	
	/** Returns the full directory path (everything except the file)
	 *
	 * Path( "/home/user/file" ).dirPath() returns "/home/user/"
	 * Path( "/home/user/" ).dirPath() returns "/home/user/"
	 * Path( "/home/user" ).dirPath() returns "/home/"
	 */
	QString dirPath() const;
	
	/** Returns the dirPath in the database form( winPath ) */
	QString dbDirPath() const;
	
	/// Returns the full path in the database form( winPath )
	QString dbPath() const;

	/// Returns a path object pointing to dirPath
	/// Path( "/home/user/file" ).dir().path() returns "/home/user/"
	/// Path( "/home/user/" ).dir().path() returns "/home/user/"
	Path dir() const;
	
	/// Returns the parent directory
	/// Path( "/home/user/file" ).parent().path() returns "/home/user/"
	/// Path( "/home/user/" ).parent().path() returns "/home/"
	Path parent() const;
	
	/// Returns true if the path points to an existing file
	bool fileExists() const;
	
	/// Returns true if the path points to an existing dir
	bool dirExists() const;
	
	/// Returns true if the path points to an existing symlink
	bool symLinkExists() const;
	
	/// Returns true if fileExists or dirExists return true
	bool exists() const;
	
	/// Makes the directory thats returned to by dirPath(),
	/// makes parents if up to 'makeParents' if they don't exist
	bool mkdir( int makeParents = 0 );
	
	/// Moves the current file or directory to dest
	bool move( const Path & dest ) const;
	
	/// Copies the current directory to dest
	bool copy( const Path & dest ) const;
	
	bool remove( bool dirRecursive = false, QString * error=0 );

	static bool copy( const QString &, const QString & );
	static bool move( const QString &, const QString & );
	static bool exists( const QString & );
	static bool mkdir( const QString & path, int makeParents=0 );
	static bool remove( const QString & path, bool dirRecursive = false, QString * error=0 );

	static QString winPath( const QString & );
	static QString unixPath( const QString & );
	static QString osPath( const QString & );
	
	static bool checkFileFree( const QString & path );

	static long long dirSize( const QString & path );

protected:
	
	QString mPath;
};

} // namespace

/// @}

#endif // PATH_H

