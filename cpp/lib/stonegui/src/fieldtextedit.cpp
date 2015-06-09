 
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
 * $Id: fieldtextedit.cpp 5411 2007-12-18 01:03:08Z brobison $
 */

#include <qmap.h>
#include <qlist.h>
#include <qvariant.h>
#include <qmenu.h>

#include "fieldtextedit.h"

FieldTextEdit::FieldTextEdit( QWidget * parent )
: QTextEdit( parent )
, mProxy( 0 )
, mChanged( false )
{
	connect( this, SIGNAL( textChanged() ), SLOT( slotTextChanged() ) );
}

FieldTextEdit::FieldTextEdit( RecordProxy * proxy, const QString & field, QWidget * parent )
: QTextEdit( parent )
, mProxy( proxy )
, mField( field )
, mChanged( false )
{
	if( mProxy ){
		slotRecordListChanged();
		connect( mProxy, SIGNAL( recordListChange() ), SLOT( slotRecordListChanged() ) );
		connect( mProxy, SIGNAL( updateRecordList() ), SLOT( slotUpdateRecordList() ) );
	}
	connect( this, SIGNAL( textChanged() ), SLOT( slotTextChanged() ) );
}

void FieldTextEdit::setField( const QString & field )
{
	mField = field;
	slotRecordListChanged();
}

QString FieldTextEdit::field() const
{
	return mField;
}

void FieldTextEdit::setProxy( RecordProxy * proxy )
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

void FieldTextEdit::slotRecordListChanged()
{
	clear();
	mValues.clear();
//	unsetPalette();
	if( mProxy && !mField.isEmpty() ){
		RecordList rl = mProxy->records();
		QList<QVariant> vals = rl.getValue( mField );
		QMap<QString, int> strings;
		for( QList<QVariant>::Iterator vi = vals.begin(); vi != vals.end(); ++vi ){
			QString s = (*vi).toString();
			if( s.isEmpty() )
				strings[QString::null]++;
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
		setPlainText( maxs );
		//if( mValues.size() > 1 )
		//	setPaletteBackgroundColor( QColor(200,200,200) );
		mOrig = maxs;
		mChanged = false;
	}
}

void FieldTextEdit::slotTextChanged()
{
	if( mOrig!=toPlainText() && !mChanged ){
		//unsetPalette();
		mChanged = true;
	}
}

void FieldTextEdit::slotUpdateRecordList()
{
	if( mChanged && mProxy ){
		mProxy->records().setValue( mField, toPlainText() );
	}
}

QMenu * FieldTextEdit::createMenu( const QPoint & /*pos*/ )
{
	QMenu * ret = new QMenu( this ); //QTextEdit::createPopupMenu( pos );
	/*
	int nextId = mFirstPopupId = ret->count() + 1;
	ret->insertSeparator();
	ret->insertItem( "No Change", nextId );
	if( !mChanged )
		ret->setItemChecked( nextId, true );
	nextId++;
	if( mValues.size() && !mValues[0].isEmpty() )
		ret->insertSeparator();
	for( QStringList::Iterator it = mValues.begin(); it != mValues.end(); ++it ){
		if( !(*it).isEmpty() )
			ret->insertItem( (*it).left(25), nextId );
		if( mChanged && *it == text() )
			ret->setItemChecked( nextId, true );
		nextId++;
	}
	connect( ret, SIGNAL( activated( int ) ), SLOT( popupItemActivated( int ) ) ); */
	return ret;
}

void FieldTextEdit::popupItemActivated( int id )
{
	qWarning( "popupItemActivated %i", id );
	if( id < mFirstPopupId )
		return;
	int pos = id - mFirstPopupId;
	if( pos == 0 ){ // No Change
		//if( mValues.size() > 1 )
		//	setPaletteBackgroundColor( QColor(200,200,200) );
		mChanged = false;
		setPlainText( mOrig );
	}
	else if( pos >= 1 ){ // One of the selected item's text
		mChanged = true;
		//unsetPalette();
		pos--;
		if( pos < (int)mValues.size() )
			setPlainText( mValues[pos] );
	}
}

