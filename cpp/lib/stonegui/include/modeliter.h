
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
 * $Id$
 */

#ifndef MODEL_ITER_H
#define MODEL_ITER_H

#include <qabstractitemmodel.h>
#include <qitemselectionmodel.h>
#include "stonegui.h"

class QTreeView;
class SuperModel;

class STONEGUI_EXPORT ModelIter
{
public:

	enum Filter {
		None,
	
		Checkable = 1,
		NotCheckable = 1 << 1,

		Tristate = 1 << 2,
		NotTristate = 1 << 3,

		Checked = 1 << 4,
		Unchecked = 1 << 5,
		PartiallyChecked = 1 << 6,

		Selectable = 1 << 7,
		NotSelectable = 1 << 8,

		Selected = 1 << 9,
		Unselected = 1 << 10,

		Editable = 1 << 11,
		NotEditable = 1 << 12,

		Enabled = 1 << 13,
		Disabled = 1 << 14,

		DragEnabled = 1 << 15,
		DragDisabled = 1 << 16,

		DropEnabled = 1 << 17,
		DropDisabled = 1 << 18,

		Recursive = 1 << 19,

		// Requires passing QTreeView
		DescendOpenOnly = 1 << 20,
		Hidden = 1 << 21,
		NotHidden = 1 << 22,

		StartAtEnd = 1 << 23,
		// Requires passing SuperModel
		DescendLoadedOnly = 1 << 24
	};

	static Filter fromCheckState( Qt::CheckState cs ) {
		return (cs==Qt::Checked) ? ModelIter::Checked : ((cs==Qt::Unchecked) ? ModelIter::Unchecked : ModelIter::PartiallyChecked);
	}

	ModelIter( QAbstractItemModel *, Filter filter = ModelIter::None, QItemSelectionModel * = 0, QTreeView * = 0 );
	
	ModelIter( SuperModel * fm, Filter filter = ModelIter::None, QItemSelectionModel * sm = 0, QTreeView * tv = 0 );
	
	ModelIter( const QModelIndex & index, Filter filter = ModelIter::None, QItemSelectionModel * = 0, QTreeView * = 0 );

	int depth() {
		return mDepth;
	}
	
	QModelIndex operator*() {
		return mCurrent;
	}

	QModelIndex current() { return mCurrent; }

	ModelIter & operator++() {
		return next();
	}
	
	ModelIter & operator--() {
		return prev();
	}

	ModelIter & next();
	ModelIter & prev();
	ModelIter & first();
	ModelIter & last();

	bool isValid() { return mCurrent.isValid(); }

	static QModelIndexList collect( QAbstractItemModel * m, ModelIter::Filter f = ModelIter::None, QItemSelectionModel * sm = 0, QTreeView * tv = 0 );
	static QModelIndexList collect( const QModelIndex & i, ModelIter::Filter f = ModelIter::None, QItemSelectionModel * sm = 0, QTreeView * tv = 0 );
	
	QModelIndex findFirst( int column, const QVariant & value, int role = Qt::DisplayRole );

protected:
	bool checkFilter( const QModelIndex & );
	Filter validateFilter( const Filter & );
	bool flagCheck( int, int, int, int );
	void init();
	bool canDescend( const QModelIndex & i );
	static QModelIndexList _collect( ModelIter iter );

	const QAbstractItemModel * mModel;
	SuperModel * mSuperModel;
	QItemSelectionModel * mSelectionModel;
	QTreeView * mTreeView;
	Filter mFilter;
	QModelIndex mCurrent;
	int mDepth;
};



#endif // MODEL_ITER_H

