
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

#include <qfile.h>
#include <qtextstream.h>
#include <qstringlist.h>
#include <qlist.h>

#include "blurqt.h"
#include "iniconfig.h"

#ifdef Q_OS_WIN
#include "windows.h"
#endif

IniConfig::IniConfig(QString configFile)
: mSection( QString::null )
{
	mFile = configFile;
	readFromFile();
}

IniConfig::IniConfig()
: mSection( QString::null )
{
}

IniConfig::~IniConfig()
{
}

void IniConfig::clear( bool sectionOnly )
{
	if( sectionOnly )
		mValMap[mSection].clear();
	else
		mValMap.clear();
}

void IniConfig::setFileName(const QString & fileName)
{
	mFile = fileName;
}

QString IniConfig::fileName() const
{
	return mFile;
}

void IniConfig::readFromFile( const QString & fileName, bool overwriteExisting )
{
	QString file = fileName;
	if( file.isEmpty() )
		file = mFile;

	if( !QFile::exists(file) ) {
		LOG_5( "Config file doesn't exist at: " + file );
		return;
	}
	
	QFile cfgFile(file);
	if( !cfgFile.open(QIODevice::ReadOnly) ) {
		LOG_5( "Unable to open config file for reading at: " + fileName );
		return;
	}
	
	QTextStream in(&cfgFile);
	while(!in.atEnd()){
		QString l = in.readLine();
		if( l.startsWith("[") && l.endsWith("]") ){
			mSection = l.mid(1,l.length()-2);
			if( mSection == "NO_SECTION" )
				mSection = QString::null;
			continue;
		}
		int equalsPos = l.indexOf('=');
		if( equalsPos < 0 )
			continue;
		QString key = l.left(equalsPos);
		QString val = l.mid(equalsPos+1);
		if( key.isEmpty() )
			continue;
		
		if( overwriteExisting || !mValMap[mSection].contains( key ) ) {
			mValMap[mSection][key] = val;
			mValMap[mSection][key.toLower()] = val;
		}
	}
	cfgFile.close();
}

bool IniConfig::writeToFile( const QString & fileName )
{
	QString filePath = fileName;
	if( filePath.isEmpty() )
		filePath = mFile;

	QFile file(filePath);
	if( !file.open(QIODevice::WriteOnly) ) {
		LOG_1( "Unable to open config file for writing at: " + filePath );
		return false;
	}
	QTextStream out(&file);
	QMap<QString, QMap<QString,QString> >::Iterator it;
	for(it = mValMap.begin(); it != mValMap.end(); ++it)
	{
		if( !it.value().isEmpty() ){
			out << '[' << (it.key().isEmpty() ? QString("NO_SECTION") : it.key()) << ']' << '\n';
			for( QMap<QString,QString>::Iterator valit = it.value().begin(); valit!=it.value().end(); ++valit )
				out << valit.key() << '=' << valit.value() << '\n';
			out << "\n";
		}
	}
	LOG_3( "Wrote config file to: " + filePath );
	return true;
}

void IniConfig::setSection( const QString & section )
{
	mSection = section;
}

void IniConfig::pushSection( const QString & section )
{
	mSectionStack.push_back( mSection );
	mSection = section;
}

void IniConfig::pushSubSection( const QString & subsection )
{
	pushSection( mSection.isEmpty() ? subsection : (mSection + ":" + subsection) );
}

QString IniConfig::currentSection() const
{
	return mSection;
}

void IniConfig::popSection()
{
	if( mSectionStack.size() ) {
		mSection = mSectionStack.back();
		mSectionStack.pop_back();
	}
}

QStringList IniConfig::sections() const
{
	QStringList ret;
	QList<QString> keys = mValMap.keys();
	foreach( QString it, keys )
		ret += it;
	return ret;
}

QStringList IniConfig::keys( const QRegExp & regEx ) const
{
	QStringList ret;
	QList<QString> keys = mValMap[mSection].keys();
	foreach( QString it, keys )
		if( regEx.isEmpty() || regEx.exactMatch( it ) )
			ret += it;
	return ret;
}

QVariant IniConfig::readValue( const QString & key, const QVariant & def ) const
{
	if( !mValMap[mSection].contains(key) ) {
		const_cast<IniConfig*>(this)->writeValue( key, def );
		return def;
	}
	return QVariant(mValMap[mSection][key]);
}

bool IniConfig::readBool(const QString & key, bool def) const
{
	if( !mValMap[mSection].contains(key) ) {
		const_cast<IniConfig*>(this)->writeBool(key,def);
		return def;
	}
	QString val = mValMap[mSection][key].toLower();
	if( val == "y" || val == "1" || val == "true" || val == "t" )
		return true;
	return false;
}

int IniConfig::readInt(const QString & key, int def) const
{
	if( !mValMap[mSection].contains(key) ) {
		const_cast<IniConfig*>(this)->writeInt(key,def);
		return def;
	}
	return mValMap[mSection][key].toInt();
}

QString IniConfig::readString(const QString & key, const QString & def) const
{
	if( !mValMap[mSection].contains(key) ) {
		const_cast<IniConfig*>(this)->writeString(key,encode(def));
		return def;
	}
	return decode(mValMap[mSection][key]);
}

QColor IniConfig::readColor(const QString & key, const QColor & def) const
{
	if( !mValMap[mSection].contains(key) ) {
		const_cast<IniConfig*>(this)->writeColor(key,def);
		return def;
	}
	QColor ret;
	ret.setNamedColor( mValMap[mSection][key] );
	return ret;
}

QFont IniConfig::readFont(const QString & key, const QFont & def) const
{
	if( !mValMap[mSection].contains(key) ) {
		const_cast<IniConfig*>(this)->writeFont(key, def);
		return def;
	}
	QFont ret;
	ret.fromString( mValMap[mSection][key] );
	return ret;
}

QSize IniConfig::readSize( const QString & key, const QSize & def ) const
{
	if( mValMap[mSection].contains( key ) ) {
		QStringList sl( mValMap[mSection][key].split(',') );
		if( sl.size() == 2 )
			return QSize( sl[0].toInt(), sl[1].toInt() );
	}
	const_cast<IniConfig*>(this)->writeSize( key, def );
	return def;
}

QRect IniConfig::readRect( const QString & key, const QRect & def ) const
{
	if( mValMap[mSection].contains( key ) ) {
		QStringList sl( mValMap[mSection][key].split(',') );
		if( sl.size() == 4 )
			return QRect( sl[0].toInt(), sl[1].toInt(), sl[2].toInt(), sl[3].toInt() );
	}
	const_cast<IniConfig*>(this)->writeRect( key, def );
	return def;
}

QList<int> IniConfig::readIntList( const QString & key, const QList<int> & def ) const
{
	if( mValMap[mSection].contains( key ) ) {
		QStringList sl( mValMap[mSection][key].split(',') );
		QList<int> ret;
		foreach( QString s, sl ) {
			bool okay;
			int i = s.toInt(&okay);
			if( okay ) ret << i;
		}
		return ret;
	}
	const_cast<IniConfig*>(this)->writeIntList( key, def );
	return def;
}

QList<uint> IniConfig::readUIntList( const QString & key, const QList<uint> & def ) const
{
	if( mValMap[mSection].contains( key ) ) {
		QStringList sl( mValMap[mSection][key].split(',') );
		QList<uint> ret;
		foreach( QString s, sl ) {
			bool okay;
			uint i = s.toUInt(&okay);
			if( okay ) ret << i;
		}
		return ret;
	}
	const_cast<IniConfig*>(this)->writeUIntList( key, def );
	return def;
}

QByteArray IniConfig::readByteArray( const QString & key, const QByteArray & def ) const
{
	if( mValMap[mSection].contains( key ) ) {
		QByteArray base64;
		base64.append( mValMap[mSection][key] );
		return QByteArray::fromBase64(base64);
	}
	const_cast<IniConfig*>(this)->writeByteArray( key, def );
	return def;
}

QDateTime IniConfig::readDateTime( const QString & key, const QDateTime & def ) const
{
	if( mValMap[mSection].contains( key ) ) {
		return QDateTime::fromString( mValMap[mSection][key] );
	}
	const_cast<IniConfig*>(this)->writeDateTime( key, def );
	return def;
}

Interval IniConfig::readInterval( const QString & key, const Interval & def ) const
{
	if( mValMap[mSection].contains( key ) ) {
		return Interval::fromString( mValMap[mSection][key] );
	}
	const_cast<IniConfig*>(this)->writeInterval( key, def );
	return def;
}

void IniConfig::writeValue(const QString & key, const QVariant & val)
{
	mValMap[mSection][key] = val.toString();
}

void IniConfig::writeBool(const QString & key, bool val)
{
	mValMap[mSection][key] = val ? "true" : "false";
}

void IniConfig::writeInt(const QString & key, int val)
{
	mValMap[mSection][key] = QString::number(val);
}

void IniConfig::writeString(const QString & key, const QString & val)
{
	mValMap[mSection][key] = encode(val);
}

void IniConfig::writeColor(const QString & key, const QColor & val)
{
	mValMap[mSection][key] = val.isValid() ? val.name() : "";
}

void IniConfig::writeFont(const QString & key, const QFont & val)
{
	mValMap[mSection][key] = val.toString();
}

void IniConfig::writeSize( const QString & key, const QSize & val )
{
	mValMap[mSection][key] = QString( "%1,%2" ).arg( val.width() ).arg( val.height() );
}

void IniConfig::writeRect( const QString & key, const QRect & val )
{
	mValMap[mSection][key] = QString( "%1,%2,%3,%4" ).arg(val.x()).arg(val.y()).arg(val.width()).arg(val.height());
}

void IniConfig::writeIntList( const QString & key, const QList<int> & val )
{
	QStringList sl;
	foreach( int i, val ) sl << QString::number(i);
	mValMap[mSection][key] = sl.join(",");
}

void IniConfig::writeUIntList( const QString & key, const QList<uint> & val )
{
	QStringList sl;
	foreach( uint i, val ) sl << QString::number(i);
	mValMap[mSection][key] = sl.join(",");
}

void IniConfig::writeByteArray( const QString & key, const QByteArray & bytes )
{
	mValMap[mSection][key] = QString( bytes.toBase64() );
}

void IniConfig::writeDateTime( const QString & key, const QDateTime & val )
{
	mValMap[mSection][key] = val.toString(Qt::ISODate);
}

void IniConfig::writeInterval( const QString & key, const Interval & val )
{
	mValMap[mSection][key] = val.toString();
}


void IniConfig::removeSection( const QString & group )
{
	mValMap.remove( group );
}

void IniConfig::renameSection( const QString & before, const QString & after )
{
	QMap<QString,QString> section;
	if( mValMap.contains( before ) ) {
		section = mValMap[before];
		removeSection(before);
		mValMap[after] = section;
	}
}

void IniConfig::copySection( const QString & sectionFrom, const QString & sectionTo, bool clearExisting )
{
	QMap<QString,QString> section;
	if( mValMap.contains( sectionFrom ) ) {
		section = mValMap[sectionFrom];
		if( clearExisting )
			removeSection( sectionTo );
		foreach( QString key, section.keys() )
			mValMap[sectionTo][key] = section[key];
	}
}

void IniConfig::removeKey( const QString & key )
{
	mValMap[mSection].remove( key );
}

QString IniConfig::encode( const QString & orig ) const
{
	QString result = orig;
	result.replace( '\n', "\\n" );
	return result;
}

QString IniConfig::decode( const QString & orig ) const
{
	QString result = orig;
	result.replace( "\\n", "\n" );
	return result;
}

