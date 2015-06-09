
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of Bach.
 *
 * Bach is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Bach is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Bach; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id: bachassettree.cpp 9408 2010-03-03 22:35:49Z brobison $
 */

#include <QPixmap>
#include <QLabel>

#include <qfile.h>
#include <qurl.h>
#include <qmessagebox.h>
#include <qapplication.h>
#include <qprogressdialog.h>
#include <recorddrag.h>

#include "bachassettree.h"
#include "bachitems.h"

//-------------------------------------------------------------------------------------------------
BachAssetTree::BachAssetTree( QWidget * parent )
:	RecordTreeView( parent )
,	mDragDropHelper( this )
{
	setIconSize(QSize(180,180));
	setItemDelegateForColumn( 2, new WrapTextDelegate( this ) );
}

//-------------------------------------------------------------------------------------------------
void BachAssetTree::dragEnterEvent( QDragEnterEvent * /*event*/ )
{
}

//-------------------------------------------------------------------------------------------------
void BachAssetTree::dragMoveEvent( QDragMoveEvent * event )
{
 	qWarning("BachAssetTree::dragMoveEvent procedure triggered");
    if (event->source() == this)
    {
         event->setDropAction(Qt::MoveAction);
         event->accept();
     }
    else
    {
         event->acceptProposedAction();
    }
}

//-------------------------------------------------------------------------------------------------
void BachAssetTree::dropEvent( QDropEvent * /*event*/ )
{
}

//-------------------------------------------------------------------------------------------------
void BachAssetTree::mousePressEvent(QMouseEvent *event)
{
    if ( mDragDropHelper.mousePressEvent( event ) )
    	return;
    RecordTreeView::mousePressEvent( event );
}

//-------------------------------------------------------------------------------------------------
void BachAssetTree::mouseReleaseEvent(QMouseEvent *event)
{
    if ( mDragDropHelper.mouseReleaseEvent( event ) )
    	return;
    RecordTreeView::mouseReleaseEvent( event );
}

//-------------------------------------------------------------------------------------------------
void BachAssetTree::mouseMoveEvent(QMouseEvent *event)
{
	if ( mDragDropHelper.mouseMoveEvent( event ) )
		return;
    RecordTreeView::mouseMoveEvent( event );
}
