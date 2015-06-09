
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
 * $Id: recordpropvaltree.h 5411 2007-12-18 01:03:08Z brobison $
 */


#ifndef RECORD_PROP_VAL_TREE_H
#define RECORD_PROP_VAL_TREE_H

#include "recordtreeview.h"
#include "recordpropvalmodel.h"

/**
 *  RecordPropValTree provides a tree view of a RecordPropValModel.
 *  See RecordPropValModel.
 */

class STONEGUI_EXPORT RecordPropValTree : public ExtTreeView
{
Q_OBJECT
public:
	RecordPropValTree( QWidget * parent );

	/**
	 *  Sets the records to be shown.
	 */
	void setRecords( const RecordList & );
	RecordList records() const;

	/**
	 *  Controls whether the values can be editted
	 *  Defaults to false
	 */
	void setEditable( bool editable );
	bool editable() const;

	void setModel( RecordPropValModel * model );
	RecordPropValModel * model() const;

	/// Creates a new RecordPropValTree with 'parent' showing 'records'.
	/// It shows it immediatly and returns control to the caller.
	static RecordPropValTree * showRecords( const RecordList & records, QWidget * parent = 0, bool editable = false );

public slots:
	void slotShowMenu( const QPoint &, const QModelIndex & );

};

#endif // RECORD_PROP_VAL_TREE_H

