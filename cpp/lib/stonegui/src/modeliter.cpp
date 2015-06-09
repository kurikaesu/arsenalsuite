
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
 * $Id: modeliter.cpp 6486 2008-05-02 01:34:01Z newellm $
 */

#include <Qt>
#include <qtreeview.h>

#include "blurqt.h"
#include "modeliter.h"
#include "supermodel.h"

ModelIter::ModelIter( QAbstractItemModel * m, Filter filter, QItemSelectionModel * sm, QTreeView * tv )
: mModel( m )
, mSuperModel( 0 )
, mSelectionModel( sm )
, mTreeView( tv )
, mFilter( filter )
{
	init();
}

ModelIter::ModelIter( SuperModel * fm, Filter filter, QItemSelectionModel * sm, QTreeView * tv )
: mModel( fm )
, mSuperModel( fm )
, mSelectionModel( sm )
, mTreeView( tv )
, mFilter( filter )
{
	init();
}
	
ModelIter::ModelIter( const QModelIndex & index, Filter filter, QItemSelectionModel * sm, QTreeView * tv )
: mModel( index.model() )
, mSuperModel( 0 )
, mSelectionModel( sm )
, mTreeView( tv )
, mFilter( filter )
{
	init();
	mCurrent = index;
}

void ModelIter::init()
{
	Filter filter = mFilter;
	mFilter = validateFilter( filter );
	if( filter & StartAtEnd )
		last();
	else
		first();
	if( mCurrent.isValid() && !checkFilter(mCurrent) ) {
		if( filter & StartAtEnd )
			prev();
		else
			next();
	}
}

int excl( int in, int flags )
{
	int tmp = in & flags;
	if( tmp && tmp != flags )
		return tmp;
	return 0;
}

ModelIter::Filter ModelIter::validateFilter( const Filter & f )
{
	int out = 0;
	out |= excl( f, (Checkable|NotCheckable) );
	out |= excl( f, (Tristate|NotTristate) );
	out |= excl( f, (Checked|Unchecked|PartiallyChecked) );
	out |= excl( f, (Selectable|NotSelectable) );
	out |= excl( f, (Editable|NotEditable) );
	out |= excl( f, (Enabled|Disabled) );
	out |= excl( f, (DragEnabled|DragDisabled) );
	out |= excl( f, (DropEnabled|DropDisabled) );
	out |= excl( f, (Hidden|NotHidden) );
	out |= excl( f, (Selected|Unselected) );
	out |= (f & (Recursive|DescendOpenOnly));
	return Filter(out);
}

bool ModelIter::flagCheck( int flags, int on, int off, int qt )
{
	return (mFilter&(on|off)) ? ( bool(mFilter&on) == bool(flags&qt) ) && ( bool(mFilter&off) != bool(flags&qt) ) : true;
}

bool ModelIter::checkFilter( const QModelIndex & i )
{
	if( mFilter & (Checked|Unchecked|PartiallyChecked) ) {
		QVariant v = mModel->data( i, Qt::CheckStateRole );
		Qt::CheckState cs = static_cast<Qt::CheckState>(v.toInt());
		if( cs == Qt::Checked && !(mFilter&Checked) )
			return false;
		if( cs == Qt::Unchecked && !(mFilter&Unchecked) )
			return false;
		if( cs == Qt::PartiallyChecked && !(mFilter&PartiallyChecked) )
			return false;
	}
	if( mSelectionModel && (mFilter & (Selected|Unselected) ) ) {
		bool selected = mSelectionModel->isSelected(i);
		if( selected && !(mFilter & Selected) )
			return false;
		if( !selected && !(mFilter & Unselected) )
			return false;
	}
	if( mTreeView && (mFilter & (Hidden|NotHidden) ) ) {
		bool hidden = mTreeView->isRowHidden( i.row(), i.parent() );
		if( hidden != bool(mFilter & Hidden) )
			return false;
		if( hidden == bool(mFilter & NotHidden) )
			return false;
	}
	if( !(mFilter & (Checkable|NotCheckable|Tristate|NotTristate|Selectable|NotSelectable|Editable|NotEditable|Enabled|Disabled|DragEnabled|DragDisabled|DropEnabled|DropDisabled)) )
		return true;
	int flags = i.model()->flags(i);
	return
		  flagCheck(flags,Checkable,NotCheckable,Qt::ItemIsUserCheckable)
		| flagCheck(flags,Tristate,NotTristate, Qt::ItemIsTristate)
		| flagCheck(flags,Selectable, NotSelectable, Qt::ItemIsSelectable)
		| flagCheck(flags,Editable, NotEditable, Qt::ItemIsEditable)
		| flagCheck(flags,Enabled, Disabled, Qt::ItemIsEnabled)
		| flagCheck(flags,DragEnabled, DragDisabled, Qt::ItemIsDragEnabled)
		| flagCheck(flags,DropEnabled, DropDisabled, Qt::ItemIsDropEnabled);
}

bool ModelIter::canDescend( const QModelIndex & i )
{
	if( !(mFilter & Recursive ) ) return false;
	if( mTreeView && (mFilter & DescendOpenOnly) && !mTreeView->isExpanded(i) )
		return false;
	if( mSuperModel && (mFilter & DescendLoadedOnly) && !mSuperModel->childrenLoaded(i) )
		return false;
	return true;
}

ModelIter & ModelIter::next()
{
	if( !mCurrent.isValid() ) {
		first();
		return *this;
	}

	do {
		enum Dir { Down, Right, UpRight, Stop };
		QModelIndex n;
		Dir d = (mFilter & Recursive) ? Down : Right;
		int depthChange = 0;
		while( d < Stop ) {
			depthChange = 0;
			switch( d ) {
				case Down:
					if( canDescend( mCurrent ) ) {
						n = mCurrent.child(0,0);
						depthChange++;
					}
					d = Right;
					break;
				case Right:
					n = mCurrent.sibling(mCurrent.row()+1,0);
					d = (mFilter & Recursive) ? UpRight : Stop;
					break;
				case UpRight:
				{
					QModelIndex tmp;
					n = mCurrent;
					while( n.isValid() && !tmp.isValid() ) {
						n = n.parent();
						depthChange--;
						tmp = n.sibling(n.row()+1,0);
					}
                    n = tmp;
					d = Stop;
					break;
				}
				default:;
			};
			if( n.isValid() ) break;
		}
		mCurrent = n;
		mDepth += depthChange;
	} while( mCurrent.isValid() && !checkFilter(mCurrent) );
	if( !mCurrent.isValid() ) mDepth = -1;
	return *this;
}

ModelIter & ModelIter::prev()
{
	if( !mCurrent.isValid() ) {
		return last();
	}

	QModelIndex p;
	do {
		enum Dir { RightDown, Up, Stop };
		Dir d = RightDown;
		while( d < Stop ) {
			switch( d ) {
				case RightDown:
					if( mCurrent.row() == 0 ) break;
					p = mCurrent.sibling(mCurrent.row()-1,0);
					while( p.model()->hasChildren(p) )
						p = p.child(p.model()->rowCount(p)-1,0);
					d = Up;
					break;
				case Up:
					p = mCurrent.parent();
					d = Stop;
					break;
				default:;
			}
			if( p.isValid() ) break;
		}
		mCurrent = p;
	} while( mCurrent.isValid() && !checkFilter(mCurrent) );
	if( !mCurrent.isValid() ) mDepth = -1;
	return *this;
}

ModelIter & ModelIter::first()
{
	mCurrent = mModel ? mModel->index(0,0) : QModelIndex();
	mDepth = mCurrent.isValid() ? 0 : -1;
	return *this;
}

ModelIter & ModelIter::last()
{
	QModelIndex i;
	int cc = mModel ? mModel->rowCount(i) : 0;
	mDepth = -1;
	while( cc > 0 ) {
		mDepth++;
		i = mModel->index(cc-1,0,i);
		cc = mModel->rowCount(i);
		if( !canDescend(i) )
			break;
	}
	mCurrent = i;
	return *this;
}

QModelIndex ModelIter::findFirst( int column, const QVariant & value, int role )
{
	for( ; isValid(); next() ) {
		QModelIndex idx = mCurrent;
		if( column != idx.column() ) idx = idx.sibling( idx.row(), column );
		if( idx.isValid() && idx.data(role) == value ) return idx;
	}
	return QModelIndex();
}

QModelIndexList ModelIter::collect( QAbstractItemModel * m, ModelIter::Filter f, QItemSelectionModel * sm, QTreeView * tv )
{
	return _collect( ModelIter(m,f,sm,tv) );
}

QModelIndexList ModelIter::collect( const QModelIndex & i, ModelIter::Filter f, QItemSelectionModel * sm, QTreeView * tv )
{
	return _collect( ModelIter(i,f,sm,tv) );
}

QModelIndexList ModelIter::_collect( ModelIter iter )
{
	QModelIndexList list;
	while( iter.isValid() ) {
		list += *iter;
		++iter;
	}
	return list;
}

