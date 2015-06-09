 
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
 * $Id: fieldlineedit.cpp 5411 2007-12-18 01:03:08Z brobison $
 */

#include <qmap.h>
#include <qlist.h>
#include <qvariant.h>
#include <qmenu.h>

#include "fieldlineedit.h"


FieldLineEdit::FieldLineEdit( RecordProxy * proxy, const QString & field, QWidget * parent )
: QLineEdit( parent )
, mProxy( proxy )
, mField( field )
{
	if( mProxy ){
		slotRecordListChanged();
		connect( mProxy, SIGNAL( recordListChange() ), SLOT( slotRecordListChanged() ) );
		connect( mProxy, SIGNAL( updateRecordList() ), SLOT( slotUpdateRecordList() ) );
	}
	connect( this, SIGNAL( textChanged( const QString & ) ), SLOT( slotTextChanged( const QString & ) ) );
}

FieldLineEdit::FieldLineEdit( QWidget * parent )
: QLineEdit( parent )
, mProxy( 0 )
{
	connect( this, SIGNAL( textChanged( const QString & ) ), SLOT( slotTextChanged( const QString & ) ) );
}

void FieldLineEdit::setField( const QString & field )
{
	mField = field;
	slotRecordListChanged();
}

QString FieldLineEdit::field() const
{
	return mField;
}

bool FieldLineEdit::changed() const
{
	return mChanged;
}

void FieldLineEdit::reset()
{
	mChanged = 0;
}

void FieldLineEdit::setProxy( RecordProxy * proxy )
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

void FieldLineEdit::slotRecordListChanged()
{
	clear();
	mValues.clear();
	if( mProxy && !mField.isEmpty() ){
		RecordList rl = mProxy->records();
		QList<QVariant> vals = rl.getValue( mField );
		QMap<QString, int> strings;
		for( QList<QVariant>::Iterator vi = vals.begin(); vi != vals.end(); ++vi ){
			QString s = (*vi).toString();
			if( s.isNull() )
				strings[""]++;
			else
				strings[s]++;
		}
		int max=0;
		QString maxs;
		for( QMap<QString,int>::Iterator mi = strings.begin(); mi != strings.end(); ++mi){
			mValues += mi.key();
			if( mi.value() > max ){
				max = mi.value();
				maxs = mi.key();
			}
		}
		setText( maxs );
		mOrig = maxs;
		mChanged = false;
	}
}

void FieldLineEdit::slotTextChanged( const QString & text )
{
	if( mOrig!=text && !mChanged ){
		//unsetPalette();
		mChanged = true;
	}
}

void FieldLineEdit::slotUpdateRecordList()
{
	if( mChanged && mProxy ){
		mProxy->records().setValue( mField, text() );
	}
}

QMenu * FieldLineEdit::createMenu()
{
	QMenu * ret = new QMenu( this );//QLineEdit::createPopupMenu();
	ret->addSeparator();
	QAction * nc = ret->addAction( "No Change" );
	if( !mChanged ) {
		nc->setCheckable( true );
		nc->setChecked( true );
	}
	if( mValues.size() && !mValues[0].isEmpty() )
		ret->addSeparator();
	for( QStringList::Iterator it = mValues.begin(); it != mValues.end(); ++it ){
		QAction * a = 0;
		if( !(*it).isEmpty() )
			a = ret->addAction( *it );
		if( mChanged && *it == text() ) {
			a->setCheckable( true );
			a->setChecked( true );
		}
	}
//	connect( ret, SIGNAL( activated( int ) ), SLOT( popupItemActivated( int ) ) );
	return ret;
}

void FieldLineEdit::popupItemActivated( int id )
{
	qWarning( "popupItemActivated %i", id );
	if( id < mFirstPopupId )
		return;
	int pos = id - mFirstPopupId;
	if( pos == 0 ){ // No Change
//		if( mValues.size() > 1 )
//			setPaletteBackgroundColor( QColor(200,200,200) );
		mChanged = false;
		setText( mOrig );
	}
	else if( pos >= 1 ){ // One of the selected item's text
		mChanged = true;
	//	unsetPalette();
		pos--;
		if( pos < (int)mValues.size() )
			setText( mValues[pos] );
	}
}

