
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
 * $Id: bachassetlistview.cpp 9408 2010-03-03 22:35:49Z brobison $
 */

#include <QPixmap>
#include <QLabel>

#include <qfile.h>
#include <qurl.h>
#include <qmessagebox.h>
#include <qapplication.h>
#include <qprogressdialog.h>

#include "bachassetlistview.h"
#include "bachitems.h"
#include "bachbucketmap.h"
#include "bachbucketmaplist.h"

extern bool bach_editor_mode;

//-------------------------------------------------------------------------------------------------
BachAssetListView::BachAssetListView( QWidget * parent )
:	RecordListView( parent )
,	mDragDropHelper( this )
,	mShowingCollection( false )
{
	setFlow(QListView::LeftToRight);
	setWrapping(true);
	setUniformItemSizes(true);
	setLayoutMode(QListView::Batched);
	setResizeMode(QListView::Adjust);
	setSelectionRectVisible(true);
}

//-------------------------------------------------------------------------------------------------
void BachAssetListView::setShowingCollection( bool a_ShowingCollection )
{
	mShowingCollection = a_ShowingCollection;
	mCurrentCollection = BachBucket();
}

//-------------------------------------------------------------------------------------------------
void BachAssetListView::setCollection( const BachBucket & a_BB, bool a_ShowExcluded )
{
	mCurrentCollection = a_BB;
	mShowExcluded = a_ShowExcluded;
}

//-------------------------------------------------------------------------------------------------
void BachAssetListView::dragEnterEvent( QDragEnterEvent * event )
{
	if ( !mShowingCollection || !bach_editor_mode )
		return;

	DEBG( "dragEnterEvent 1!" );
    if (event->source() == this)
    {
    	DEBG( "dragEnterEvent 2.1!" );
        event->setDropAction(Qt::MoveAction);
        event->accept();
    }
    else
    {
    	DEBG( "dragEnterEvent 2.2!" );
        event->acceptProposedAction();
    }

}

//-------------------------------------------------------------------------------------------------
void BachAssetListView::dragLeaveEvent( QDragEnterEvent * /*event*/ )
{
	if ( !mShowingCollection || !bach_editor_mode)
		return;

	mCurrentDropTarget = BachAsset();
	// nothing
}

//-------------------------------------------------------------------------------------------------
void BachAssetListView::dragMoveEvent( QDragMoveEvent * event )
{
	if ( !mShowingCollection || !bach_editor_mode )
		return;

    if (event->source() == this)
	{
		QModelIndex midx = indexAt( event->pos() );
		mCurrentDropTarget = model()->getRecord( midx );
    	DEBG( "dropEvent 2.2!"+mCurrentDropTarget.path() );
		// setSelection( RecordList( mCurrentDropTarget ) );
		event->acceptProposedAction();
	}
}

//-------------------------------------------------------------------------------------------------
void BachAssetListView::dropEvent( QDropEvent * event )
{
	if ( !mShowingCollection || !bach_editor_mode )
		return;

	DEBG( "dropEvent 1!" );
    if ( event->source() == this &&
    	 mCurrentDropTarget.isValid() &&
    	 mCurrentCollection.isValid() )
    {
    	DEBG( "dropEvent 2.2!" );

    	if ( !RecordDrag::canDecode( event->mimeData() ) )
    		return;

    	BachAssetList bal;
    	GetAssets( mCurrentCollection, mShowExcluded, bal );


    	RecordList dropped;
    	RecordDrag::decode( event->mimeData(), &dropped );

    	BachAssetIter it = bal.begin();

    	// first, do all the items UP to the item into which it should be inserted before
    	int position = 0;
    	for( ; it != bal.end() ; ++it )
    	{
    		BachAsset ba = (*it);

    		if ( mCurrentDropTarget == ba )
    			break;

    		if ( dropped.contains( ba ) )
    			continue;

    		BachBucketMap bbm = BachBucketMap::recordByBucketAndAsset( mCurrentCollection, ba );
    		DEBG( "Position1:"+QString::number(position)+":"+ba.path() );

    		bbm.setPosition( position );
    		bbm.commit();

    		++position;
    	}

    	// then do all the dropped ones
    	for ( RecordIter it = dropped.begin() ; it != dropped.end() ; ++it )
    	{
    		BachAsset ba = *it;
    		// find the map used
    		BachBucketMap bbm = BachBucketMap::recordByBucketAndAsset( mCurrentCollection, ba );

    		DEBG( "Position2:"+QString::number(position)+":"+bbm.bachAsset().path() );

    		bbm.setPosition( position );
    		bbm.commit();

    		++position;
    	}

    	// then do all the ones past the end
    	for( ; it != bal.end() ; ++it )
    	{
    		BachAsset ba = (*it);

    		if ( dropped.contains( ba ) )
    			continue;

    		BachBucketMap bbm = BachBucketMap::recordByBucketAndAsset( mCurrentCollection, ba );
    		DEBG( "Position3:"+QString::number(position)+":"+ba.path() );

    		bbm.setPosition( position );
    		bbm.commit();

    		++position;
    	}

    	BachAssetList bal1;
    	GetAssets( mCurrentCollection, mShowExcluded, bal1 );

    	model()->updateRecords( BachAssetList() );
    	model()->append( bal1 );
        event->acceptProposedAction();
    }
}

//-------------------------------------------------------------------------------------------------
void BachAssetListView::mousePressEvent(QMouseEvent * event)
{
	// DEBG( "mousePressEvent 1!" );
    if ( bach_editor_mode && mDragDropHelper.mousePressEvent( event ) )
    	return;
    RecordListView::mousePressEvent( event );
}


//-------------------------------------------------------------------------------------------------
void BachAssetListView::mouseReleaseEvent(QMouseEvent * event)
{
	// DEBG( "mouseReleaseEvent 1!" );
    if ( bach_editor_mode && mDragDropHelper.mouseReleaseEvent( event ) )
    	return;
    RecordListView::mouseReleaseEvent( event );
}

//-------------------------------------------------------------------------------------------------
void BachAssetListView::mouseMoveEvent(QMouseEvent * event)
{
	// DEBG( "mouseMoveEvent 1!" );
	if ( bach_editor_mode && mDragDropHelper.mouseMoveEvent( event ) )
		return;
	RecordListView::mousePressEvent( event );
}
