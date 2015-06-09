
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
 * $Id: renderelements.cpp 5409 2007-12-18 00:32:50Z brobison $
 */

#include <qfile.h>
#include <qdir.h>
#include <qregexp.h>

#include "path.h"
#include "renderelements.h"

RenderElement::RenderElement( const QString & fileName )
: mFile( fileName )
{
	if( QFile::exists( fileName ) ){
		mConfig.setFileName( fileName );
		mConfig.readFromFile();
	}
}

QStringList RenderElement::lightGroups()
{
	return mConfig.sections().filter( QRegExp( "^LIGHT_" ) );
}

QStringList RenderElement::layerGroups()
{
	return mConfig.sections().filter( QRegExp( "^LAYER_" ) );
}

QStringList RenderElement::atmosphericsGroups()
{
	return mConfig.sections().filter( QRegExp( "^ATMOSPHERIC_" ) );
}

QStringList RenderElement::effectsGroups()
{
	return mConfig.sections().filter( QRegExp( "^EFFECT_" ) );
}

QStringList RenderElement::allGroups()
{
	return mConfig.sections();
}

QStringList RenderElement::keys()
{
	return mConfig.keys();
}

QString RenderElement::group()
{
	return mConfig.currentSection();
}

void RenderElement::setGroup( const QString & section )
{
	mConfig.setSection( section );
}

QString RenderElement::getString( const QString & property )
{
	return mConfig.readString( property );
}

int RenderElement::getInt( const QString & property )
{
	return mConfig.readInt( property );
}

bool RenderElement::getBool( const QString & property )
{
	return mConfig.readBool( property );
}

void RenderElement::setString( const QString & property, const QString & val )
{
	mConfig.writeString( property, val );
}

void RenderElement::setInt( const QString & property, int val )
{
	mConfig.writeInt( property, val );
}

void RenderElement::setBool( const QString & property, bool val )
{
	mConfig.writeBool( property, val );
}

void RenderElement::removeKey( const QString & property )
{
	mConfig.removeKey( property );
}

void RenderElement::removeGroup( const QString & group )
{
	mConfig.removeSection( group );
}

void RenderElement::save()
{
	mConfig.writeToFile();
}

QString RenderElement::file() const
{
	return mFile;
}

RenderElements::RenderElements( const QString & path )
: mPath( path )
{
	QDir dir( path );
	if( dir.exists() ){
		QStringList els = dir.entryList( QStringList() << "*.ini", QDir::Files );
		for( QStringList::Iterator it = els.begin(); it != els.end(); ++it )
		{
			mElementMap[QString(*it).replace(".ini", "")] = 0;
		}
	}
}

RenderElements::~RenderElements()
{
	for( REIter it = mElementMap.begin(); it != mElementMap.end(); ++it )
		delete *it;
}

RenderElement * RenderElements::addElement( const QString & name )
{
	RenderElement * ret = new RenderElement( mPath + "/" + name );
	mElementMap[name] = ret;
	return ret;
}

void RenderElements::removeElement( const QString & name )
{
	if( mElementMap.contains( name ) ){
		delete mElementMap[name];
		mElementMap.remove( name );
		QFile::remove( mPath + "/" + name + ".ini" );
	}
}

RenderElement * RenderElements::elementByName( const QString & name )
{
	if( mElementMap.contains( name ) ){
		RenderElement * ret = mElementMap[name];
		if( !ret )
			ret = mElementMap[name] = new RenderElement( mPath + "/" + name + ".ini" );
		return ret;
	}
	return 0;
}

QStringList RenderElements::elementNameList()
{
	QStringList ret;
	for( REIter it = mElementMap.begin(); it != mElementMap.end(); ++it )
		ret += it.key();
	return ret;
}

QString renderElementDir( const QString & maxFilePath )
{
	QString eldir = maxFilePath;
	eldir = eldir.replace( ".max", "" ) + ".render_elements/";
	if( Path( eldir ).dirExists() )
		return eldir;
	eldir = Path( maxFilePath ).dirPath() + "/render_elements/";
	if( Path( eldir ).dirExists() )
		return eldir;
	return QString::null;
}

