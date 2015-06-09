
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
 * $Id: renderelements.h 5409 2007-12-18 00:32:50Z brobison $
 */

#ifndef RENDER_ELEMENTS_H
#define RENDER_ELEMENTS_H


#include <qstringlist.h>

#include "iniconfig.h"

class STONE_EXPORT RenderElement
{
public:
	RenderElement( const QString & fileName );

public slots:
	QStringList lightGroups();
	QStringList layerGroups();
	QStringList effectsGroups();
	QStringList atmosphericsGroups();
	QStringList allGroups();

	QString group();
	void setGroup( const QString & );
	void removeGroup( const QString & group );

	QStringList keys();

	QString getString( const QString & property );
	int getInt( const QString & property );
	bool getBool( const QString & property );

	void setString( const QString & property, const QString & val );
	void setInt( const QString & property, int val );
	void setBool( const QString & property, bool val );

	void removeKey( const QString & property );
	
	void save();
	
	QString file() const;
protected:
	IniConfig mConfig;
	QString mFile;
};

class STONE_EXPORT RenderElements
{
public:
	RenderElements( const QString & path );

	~RenderElements();
	
public slots:
	RenderElement * addElement( const QString & name );

	RenderElement * elementByName( const QString & name );

	void removeElement( const QString & name );

	QStringList elementNameList();

protected:
	typedef QMap<QString, RenderElement*> REMap;
	typedef QMap<QString, RenderElement*>::Iterator REIter;
	REMap mElementMap;
	QString mPath;
};


STONE_EXPORT QString renderElementDir( const QString & maxFilePath );
	 

#endif // RENDER_ELEMENTS_H

