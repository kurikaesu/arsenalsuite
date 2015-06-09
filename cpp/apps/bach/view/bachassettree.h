
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
 * $Id: bachassettree.h 9408 2010-03-03 22:35:49Z brobison $
 */

#ifndef BACH_ASSET_TREE_H
#define BACH_ASSET_TREE_H

#include <qevent.h>
#include <qwidget.h>

#include "recordtreeview.h"

#include "bachasset.h"
#include "DragDropHelper.h"

class BachAssetTree : public RecordTreeView
{
Q_OBJECT
public:
	BachAssetTree( QWidget * parent );

protected:
	void dragEnterEvent( QDragEnterEvent * event );
	void dragMoveEvent( QDragMoveEvent * event );
	void dropEvent( QDropEvent * event );
    void mousePressEvent( QMouseEvent *event );
    void mouseReleaseEvent( QMouseEvent *event );
    void mouseMoveEvent( QMouseEvent *event );
private:
	DragDropHelper<BachAssetTree> mDragDropHelper;
};

#endif // ELEMENT_LIST_VIEW_H

