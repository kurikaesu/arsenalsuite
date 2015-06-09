
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
 * $Id: recordlistview.cpp 13650 2012-10-01 22:12:31Z newellm $
 */

#include "recordlistview.h"
#include "recordsupermodel.h"

RecordListView::RecordListView( QWidget * parent )
: QListView( parent )
{
}

void RecordListView::setModel( RecordSuperModel * model )
{
	QListView::setModel( model );
	QItemSelectionModel * sm = selectionModel();
	connect( sm, SIGNAL( currentChanged( const QModelIndex &, const QModelIndex & ) ),
		SLOT( slotCurrentChanged( const QModelIndex &, const QModelIndex & ) ) );
	connect( sm, SIGNAL( selectionChanged( const QItemSelection &, const QItemSelection & ) ),
		SLOT( slotSelectionChanged( const QItemSelection &, const QItemSelection & ) ) );
}

RecordSuperModel * RecordListView::model() const
{
	return qobject_cast<RecordSuperModel*>(QListView::model());
}

Record RecordListView::current()
{
	QModelIndex idx = selectionModel()->currentIndex();
	return idx.isValid() ? model()->getRecord( idx ) : Record();
}

RecordList RecordListView::selection()
{
	return model()->listFromIS( selectionModel()->selection() );
}

void RecordListView::setSelection( const RecordList & rl )
{
	QItemSelectionModel * sm = selectionModel();
	sm->clear();
	QModelIndexList il = model()->findIndexes(rl);
	foreach( QModelIndex i, il )
		sm->select( i, QItemSelectionModel::Select );
}

void RecordListView::setCurrent( const Record & r )
{
	QModelIndex i = model()->findIndex( r );
	if( i.isValid() )
		selectionModel()->setCurrentIndex( i, QItemSelectionModel::NoUpdate );
}

void RecordListView::slotCurrentChanged( const QModelIndex & i, const QModelIndex & )
{
	emit currentChanged( model()->getRecord(i) );
}

void RecordListView::slotSelectionChanged( const QItemSelection & sel, const QItemSelection & )
{
	emit selectionChanged( model()->listFromIS( sel ) );
}

