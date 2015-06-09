
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

#include "genericimp.h"

GenericImp::GenericImp( Table * table, QVariant * toLoad )
: RecordImp( true )
, mTable( table )
, mValues( 0 )
{
	int cc = table->columnCount();
	mValues = new VariantVector( cc );
	while( --cc >= 0 && toLoad )
		mValues->at( cc ) = toLoad[cc];
}

GenericImp::~GenericImp()
{
	delete mValues;
	mValues = 0;
}
	
int GenericImp::get( QVariant * v )
{
	int cc = mTable->columnCount();
	while(--cc >= 0 )
		v[cc] = mValues->at( cc );
	return 0;
}

int GenericImp::set( QVariant * v )
{
	int cc = mTable->columnCount();
	while(--cc >= 0 )
		mValues->at( cc ) = v[cc];
	return 0;
}

QVariant GenericImp::getColumn( int col ) const
{
	if( col >= (int)mTable->columnCount() )
		return QVariant();
	return mValues->at(col);
}

void GenericImp::setColumn( int col, const QVariant & v )
{
	if( col >= (int)mTable->columnCount() )
		return;
	mValues->at(col) = v;
}

uint GenericImp::getUIntColumn( int col ) const
{
	if( col >= (int)mTable->columnCount() )
		return 0;
	return mValues->at(col).toInt();
}

void GenericImp::setUIntColumn( int col, uint val )
{
	if( col >= (int)mTable->columnCount() )
		return;
	mValues->at(col) = QVariant(val);
}

Table * GenericImp::table() const
{
	return mTable;
}

RecordImp * GenericImp::copy()
{
	GenericImp * t = new GenericImp( mTable, &mValues->at(0) );
	t->mKey = 0;
	t->mRefCount = 0;
	t->mUpdated = 0;
	return t;

}

