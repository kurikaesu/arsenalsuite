
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
 * $Id: recordpropvalmodel.h 11912 2011-07-27 18:31:20Z newellm $
 */

#ifndef RECORD_PROP_VAL_MODEL_H
#define RECORD_PROP_VAL_MODEL_H

#include "blurqt.h"

#include "recordsupermodel.h"
#include "field.h"

/**
 *  This class implements the QAbstractItemModel interface.
 *  It provides a two column model, with Property(Column) | Value.
 *  Each row represents 1 column in the table associated with 
 *  the current records, set via setRecords.
 *  It provides optional editting of values, enabled with setEditable.
 */
class STONEGUI_EXPORT RecordPropValModel : public SuperModel
{
Q_OBJECT
public:
	RecordPropValModel( QObject * parent );

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

	void setMultiEdit( bool );
	bool multiEdit() const;

	/// If the modelindex is a value column, that is showing one or more foreign key
	/// records, this function will return those records.
	RecordList foreignKeyRecords( const QModelIndex & );

protected:
	RecordList mRecords;
	bool mEditable;
	friend class PropValItem;
	bool mMultiEdit;
};


#endif // RECORD_PROP_VAL_MODEL_H

