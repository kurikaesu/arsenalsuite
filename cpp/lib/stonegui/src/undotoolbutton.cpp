
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
 * $Id: undotoolbutton.cpp 12771 2012-02-21 18:03:54Z newellm $
 */

#include <qmenu.h>

#include "blurqt.h"
#include "database.h"
#include "undotoolbutton.h"
#include "recordimp.h"
#include "table.h"

UndoRedoToolButton::UndoRedoToolButton( QWidget * parent, QUndoStack * undoStack, bool isUndo )
: QToolButton( parent )
, mIsUndo( isUndo )
, mUndoStack( undoStack )
{
	setToolTip( isUndo ? "Undo" : "Redo" );
	setIcon( QIcon( isUndo ? ":/images/undo.png" : ":/images/redo.png" ) );
	setIconSize( QSize( 22, 22 ) );
	if( isUndo )
		connect( mUndoStack, SIGNAL( canUndo( bool ) ), SLOT( canUndoRedoChange( bool ) ) );
	else
		connect( mUndoStack, SIGNAL( canRedo( bool ) ), SLOT( canUndoRedoChange( bool ) ) );
	connect( this, SIGNAL( clicked() ), mUndoStack, isUndo ? SLOT( undo() ) : SLOT( redo() ) );
	setEnabled( isUndo ? mUndoStack->canUndo() : mUndoStack->canRedo() );
}


void UndoRedoToolButton::updatePopup()
{
	QMenu * pp = new QMenu( this );
	if( !pp ) {
		pp = new QMenu( this );
		connect( pp, SIGNAL( triggered( QAction * ) ), SLOT( menuItemClicked( QAction * ) ) );
	}
	pp->clear();
	int step = mIsUndo ? -1 : 1;
	int stop = qBound(0, mUndoStack->index() + (mIsUndo ? -10 : 10), mUndoStack->count());
	for( int i = mUndoStack->index() + step; i != stop; i += step ) {
		QAction * act = pp->addAction( mUndoStack->text(i) );
		act->setData( i );
	}
	setMenu( pp );
}

void UndoRedoToolButton::canUndoRedoChange( bool canUndoRedo )
{
	setEnabled( canUndoRedo );
	updatePopup();
}

void UndoRedoToolButton::menuItemClicked( QAction * act )
{
	mUndoStack->setIndex(act->data().toInt());
}

