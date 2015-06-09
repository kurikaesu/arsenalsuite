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
#include "checklistitem.h"
#include "checkliststatus.h"
#include "element.h"
#include "elementstatus.h"
#include "statusset.h"

ElementStatus CheckListItem::status( const Element & element ) const
{
	return CheckListStatus::recordByElementAndItem( element, *this ).status();
}

void CheckListItem::setStatus( const Element & element, const ElementStatus & status ) const
{
	CheckListStatus cls = CheckListStatus::recordByElementAndItem( element, *this );
	if( !cls.isRecord() ) {
		cls.setCheckListItem( *this );
		cls.setElement( element );
	}
	if( status.statusSet() != statusSet() )
		return;
	cls.setStatus( status );
	cls.commit();
}

QString CheckListItem::statusString( const Element & element ) const
{
	return status( element ).name();
}

QString CheckListItem::summary( const Element & )
{
/*	uint cl_items=0, cl_done=0, cl_approved=0;
	CheckListItemList cl = CheckListItem::recordsByAssetTypeAndProject( e.assetType(), e.project() );
	st_foreach( CheckListIter, it, cl )
	{
		cl_items++;
		if( (*it).state( e ) & 1 )
			cl_done++;
		if( (*it).state( e ) & 2 )
			cl_approved++;
	}
	return QString("%1, %2, %3").arg( cl_items ).arg( cl_done ).arg( cl_approved ); */
	return QString();
}

#endif

