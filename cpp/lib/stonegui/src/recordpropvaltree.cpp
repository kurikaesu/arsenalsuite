
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
 * $Id: recordpropvaltree.cpp 9389 2010-02-25 00:22:37Z brobison $
 */

#include <qmenu.h>

#include "recordpropvaltree.h"
#include "table.h"

RecordPropValTree::RecordPropValTree( QWidget * parent )
: ExtTreeView( parent )
{
	setModel( new RecordPropValModel(this) );
	connect( this, SIGNAL( showMenu( const QPoint &, const QModelIndex & ) ), SLOT( slotShowMenu( const QPoint &, const QModelIndex & ) ) );
}

void RecordPropValTree::setRecords( const RecordList & rl )
{
	model()->setRecords(rl);
}

RecordList RecordPropValTree::records() const
{
	return model()->records();
}

void RecordPropValTree::setEditable( bool editable )
{
	model()->setEditable(editable);
}

bool RecordPropValTree::editable() const
{
	return model()->editable();
}

void RecordPropValTree::setModel( RecordPropValModel * model )
{
	ExtTreeView::setModel(model);
}

RecordPropValModel * RecordPropValTree::model() const
{
	return qobject_cast<RecordPropValModel*>(this->ExtTreeView::model());
}

RecordPropValTree * RecordPropValTree::showRecords( const RecordList & records, QWidget * parent, bool editable )
{
	RecordPropValTree * tree = new RecordPropValTree(parent);
	tree->setRecords( records );
	tree->setEditable( editable );
	tree->show();
	return tree;
}

void RecordPropValTree::slotShowMenu( const QPoint & pos, const QModelIndex & index )
{
	RecordList dest = model()->foreignKeyRecords( index );
	if( dest.isEmpty() ) return;

	QMenu * menu = new QMenu( this );
	QAction * go = menu->addAction( "Show " + dest[0].table()->schema()->className() );
	
	QAction * result = menu->exec( pos );
	delete menu;

	if( result == go ) {
		setRecords(dest);
	}
}

