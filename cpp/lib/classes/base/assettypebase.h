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

#ifdef HEADER_FILES
#include "element.h"
#include "project.h"
#endif

#ifdef CLASS_FUNCTIONS
	/**
	 *  Returns an AssetTemplate used to construct this AssetType.
	 *  Looks first for a template named 'Default' that is assigned
	 *  to project, if project is a valid record.  If project is not
	 *  a valid record, or there is no 'Default' template for the
	 *  project, then the global templates are searched for a
	 *  a default template.  If no project or global 'Default' template
	 *  is found, then an empty record is returned
	 **/
	AssetTemplate findDefaultTemplate( const Project & project = Project() );

	/**
	 *  Returns a newly created(not committed) element of this asset type.
	 *  The assettype and element type are set according to this asset type,
	 *  and the name is set to the name of the asset type.
	 **/
	Element construct();

	/**
	 *  Returns a top-level asset group for a project(element.project())
	 *  Returns Element() if none is found.
	 **/
	Element getGroupElement( const Element & element );

	/** Returns the AssetType for a character */
	static AssetType character();

	/** Returns the AssetType for a prop */
	static AssetType prop();

	/** Returns the AssetType for an environment */
	static AssetType environment();

	/** Returns the AssetType for a camera */
	static AssetType camera();

	/** Returns the AssetType for a light */
	static AssetType light();
	
	/** Filters a list of asset types according to their tags() field.  Each asset type
	 *  is returned only if it's tags() field contains all the tags listed in the 
	 *  inputted tags string list.
	 **/
	static AssetTypeList filterByTags( const AssetTypeList & list, const QStringList & tags );

	/**
	 *  Returns all asset types in the database that contain the inputted tags.
	 *  If enabledOnly is true the disabled asset types are filtered out.
	 *  This is the same as  AssetType::filterByTags( AssetType::select()[.filter("disabled",0)], tags )
	 **/
	static AssetTypeList recordsByTags( const QStringList & tags, bool enabledOnly = true );

#endif

