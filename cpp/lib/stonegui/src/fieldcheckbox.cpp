
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
 * $Id: fieldcheckbox.cpp 5411 2007-12-18 01:03:08Z brobison $
 */

#include <qset.h>

#include "fieldcheckbox.h"

FieldCheckBox::FieldCheckBox( QWidget * parent )
: QCheckBox( parent )
{}

void FieldCheckBox::setField( const QString & field )
{
	mField = field;
}

QString FieldCheckBox::field() const
{
	return mField;
}

void FieldCheckBox::setProxy( RecordProxy * proxy )
{
	if( mProxy )
		mProxy->disconnect( this );
	mProxy = proxy;
	if( mProxy ){
		slotRecordListChanged();
		connect( mProxy, SIGNAL( recordListChange() ), SLOT( slotRecordListChanged() ) );
		connect( mProxy, SIGNAL( updateRecordList() ), SLOT( slotUpdateRecordList() ) );
	}
}
	
void FieldCheckBox::slotRecordListChanged()
{
	if( mProxy && !mField.isEmpty() ){
		RecordList rl = mProxy->records();
		QList<QVariant> vals = rl.getValue( mField );
		QList<bool> boolVals;
		foreach( QVariant v, vals ) {
			boolVals.append( v.toBool() );
		}
		// get unique values
		boolVals = boolVals.toSet().toList();
		Qt::CheckState checkState = boolVals.size() == 1 ? (boolVals[0] ? Qt::Checked : Qt::Unchecked) : Qt::PartiallyChecked;
		setTristate( checkState == Qt::PartiallyChecked );
		setCheckState( checkState );
	}
}

void FieldCheckBox::slotUpdateRecordList()
{
	Qt::CheckState cs = checkState();
	if( cs != Qt::PartiallyChecked ) {
		mProxy->records().setValue( mField, bool(cs == Qt::Checked) );
	}
}

