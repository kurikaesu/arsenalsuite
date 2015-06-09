/*
 *
 * Copyright 2003, 2004 Blur Studio Inc.
 *
 * This file is part of the Resin software package.
 *
 * Resin is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Resin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Resin; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef COMMIT_CODE
#include "assettype.h"
#include "project.h"
#include "database.h"
#include "table.h"
#include "assettemplate.h"

AssetTemplate AssetType::findDefaultTemplate( const Project & project )
{
	AssetTemplateList templates;
	// First look for project specific 'Default' template
	if( project.isRecord() )
		templates = AssetTemplate::recordsByProjectAndAssetType( project, *this ).filter("name","Default");
	// If there are none, then look for global 'Default' template
	if( templates.isEmpty() )
		templates = assetTemplates().filter("name","Default");
	if( templates.size() == 1 )
		return templates[0];
	return AssetTemplate();
}

Element AssetType::construct()
{
	Element e;
	Table * t = Database::current()->tableByName( elementType().name() );
	if( t )
		e = t->load();
	e.setAssetType( *this );
	e.setElementType( elementType() );
	e.setName( name() );
	return e;
}

Element AssetType::getGroupElement( const Element & e )
{
	Element ret;
	Project p = e.project();
	if( p.isRecord() ){
		ElementList elist = p.children( ElementType::assetGroupType() );
		foreach( Element e, elist )
			if( e.name()==(name()+"s") )
				ret = e;
	}
	return ret;
}

AssetType AssetType::character()
{
	return AssetType::recordByName( "Character" );
}

AssetType AssetType::prop()
{
	return AssetType::recordByName( "Prop" );
}

AssetType AssetType::environment()
{
	return AssetType::recordByName( "Environment" );
}

AssetType AssetType::camera()
{
	return AssetType::recordByName( "Camera" );
}

AssetType AssetType::light()
{
	return AssetType::recordByName( "Light" );
}

AssetTypeList AssetType::filterByTags( const AssetTypeList & list, const QStringList & tags )
{
	AssetTypeList ret(list);
	foreach( QString tag, tags )
		if( !tag.isEmpty() )
			ret = ret.filter( "tags", QRegExp(tag) );
	return ret;
}

AssetTypeList AssetType::recordsByTags( const QStringList & tags, bool enabledOnly )
{
	AssetTypeList ret = AssetType::select();
	if( enabledOnly )
		ret = ret.filter( "disabled", 0 );
	return filterByTags(ret,tags);
}

#endif

