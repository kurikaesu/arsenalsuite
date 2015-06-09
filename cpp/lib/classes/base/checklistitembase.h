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
#include "project.h"

class CheckListItem;
class CheckListItemList;
typedef CheckListItemList CheckList;
class CheckListItemIter;
typedef CheckListItemIter CheckListIter;
class Element;
#endif

#ifdef CLASS_FUNCTIONS

	ElementStatus status( const Element & ) const;

	void setStatus( const Element &, const ElementStatus & ) const;

	QString statusString( const Element & ) const;

	static QString summary( const Element & );

/*	static CheckListItemList list( const Element & );

	static CheckListItemList defaults( const TaskType &, const Project & p= Project() );

	static void setDefaults( CheckListItemList items, const TaskType &, const Project & p= Project() );

*/
#endif

