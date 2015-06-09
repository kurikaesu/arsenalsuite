
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

#ifndef LIB_BLUR_QT_INI_CONFIG_H
#define LIB_BLUR_QT_INI_CONFIG_H

#include <qstring.h>
#include <qmap.h>
#include <qcolor.h>
#include <qdatetime.h>
#include <qapplication.h>
#include <qfont.h>
#include <qrect.h>
#include <qregexp.h>
#include <qvariant.h>

#include "blurqt.h"
#include "interval.h"

/**
 * \ingroup Stone
 */

class STONE_EXPORT IniConfig
{
public:
	/** Sets the fileName to `configFile', then calls readFromFile **/
	IniConfig(QString configFile);
	IniConfig();
	~IniConfig();

	/** Clears all values from the file, or the section if sectionOnly=true */
	void clear(bool sectionOnly=false);

	/** Sets the file name to operate on.
	 *  This is the default file for both readFromFile and writeToFile
	 */
	void setFileName(const QString & fileName);
	QString fileName() const;

	/** Reads all values from the file, over-writes any existing entries in memory if overwriteExisting is true.
	 *  If fileName is empty, uses the fileName last set with setFileName. */
	void readFromFile( const QString & fileName = QString(), bool overwriteExisting = true );

	/** Clears the file and writes out all values 
	 *  If fileName is empty, uses the fileName last set with setFileName. */
	bool writeToFile( const QString & fileName = QString() );

	/** Sets or gets the current section
	 *  You add a new section by calling setSection
	 *  and then writing a values
	 */
	void setSection( const QString & section );
	void pushSection( const QString & section );
	void pushSubSection( const QString & subsection );
	void popSection();
	QString currentSection() const;

	/** Returns a list of existing sections */
	QStringList sections() const;

	/** Returns a list of the keys that have been set for this section
	 * Filters the list by filterExp if it is not null */
	QStringList keys( const QRegExp & filterExp = QRegExp() ) const;

	/** Reads entries from the current section */
	QVariant readValue( const QString & key, const QVariant & def=QVariant() ) const;
	bool readBool( const QString & key, bool def=false) const;
	int readInt( const QString & key, int def=0) const;
	QString readString( const QString & key, const QString & def=QString::null) const;
	QColor readColor( const QString & key, const QColor & def=Qt::black) const;
	QFont readFont( const QString & key, const QFont & def=qApp->font()) const;
	QSize readSize( const QString & key, const QSize & def=QSize( 16,16 ) ) const;
	QRect readRect( const QString & key, const QRect & def=QRect() ) const;
	QList<int> readIntList( const QString & key, const QList<int> & def = QList<int>() ) const;
	QList<uint> readUIntList( const QString & key, const QList<uint> & def = QList<uint>() ) const;
	QByteArray readByteArray( const QString & key, const QByteArray & def = QByteArray() ) const;
	QDateTime readDateTime( const QString & key, const QDateTime & def = QDateTime() ) const;
	Interval readInterval( const QString & key, const Interval & def = Interval() ) const;
	
	void writeValue( const QString & key, const QVariant & val);
	/** Writes entries to the current section */
	void writeBool( const QString & key, bool val);
	/// Sets \param key to the int value \param val in the current section
	void writeInt( const QString & key, int val);
	/// Sets \param key to the QString value \param val in the current section
	void writeString( const QString & key, const QString & val);
	/// Sets \param key to the QColor value \param val in the current section
	void writeColor( const QString & key, const QColor & val);
	/// Sets \param key to the QFont value \param val in the current section
	void writeFont( const QString & key, const QFont & val);
	/// Sets \param key to the QSize value \param val in the current section
	void writeSize( const QString & key, const QSize & val );
	/// Sets key to the QRect value val in the current section
	void writeRect( const QString & key, const QRect & val );
	void writeIntList( const QString & key, const QList<int> & val );
	void writeUIntList( const QString & key, const QList<uint> & val );
	void writeByteArray( const QString & key, const QByteArray & val );
	void writeDateTime( const QString & key, const QDateTime & val );
	void writeInterval( const QString & key, const Interval & val );
	
	/// Removes the key/value pair that matches \param key from the current section
	void removeKey( const QString & key );

	/// Removes the entire section named \param group from this file.
	void removeSection( const QString & group );
	/// Removes any existing entries from \param dest and renames \param source to \param dest
	void renameSection( const QString & source, const QString & dest );
	/// Copies all entries from \param source to \param dest, removes any existing entries in \param dest if \param clearExisting is true
	void copySection( const QString & source, const QString & dest, bool clearExisting = true );
	
private:
	// Prepares multiline strings to be properly stored in an ini file
	QString encode( const QString & ) const;
	QString decode( const QString & ) const;

	QMap<QString, QMap<QString,QString> > mValMap;
	QString mSection;
	QStringList mSectionStack;
	QString mFile;
};


#endif // LIB_BLUR_QT_INI_CONFIG_H

