
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
 * $Id: bachassetlistview.h 9408 2010-03-03 22:35:49Z brobison $
 */

#ifndef BACH_ASSET_LIST_VIEW_H
#define BACH_ASSET_LIST_VIEW_H

#include <qevent.h>
#include <qwidget.h>

#include "recordlistview.h"

#include "bachasset.h"
#include "bachbucket.h"
#include "DragDropHelper.h"

class BachAssetListView : public RecordListView
{
Q_OBJECT

public:
	BachAssetListView( QWidget * parent );
	void setShowingCollection( bool a_ShowingCollection );
	void setCollection( const BachBucket & a_BB, bool a_ShowExcluded );
protected:
	void dragEnterEvent( QDragEnterEvent * event );
	void dragLeaveEvent( QDragEnterEvent * event );
	void dragMoveEvent( QDragMoveEvent * event );
	void dropEvent( QDropEvent * event );
    void mousePressEvent( QMouseEvent *event );
    void mouseReleaseEvent( QMouseEvent *event );
    void mouseMoveEvent( QMouseEvent *event );
private:
	DragDropHelper<BachAssetListView> mDragDropHelper;

	bool mShowingCollection;
	BachAsset mCurrentDropTarget;
	BachBucket mCurrentCollection;
	bool mShowExcluded;

};

#endif // ELEMENT_LIST_VIEW_H

