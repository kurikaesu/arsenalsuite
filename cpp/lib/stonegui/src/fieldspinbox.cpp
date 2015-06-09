 
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
 * $Id: fieldspinbox.cpp 5411 2007-12-18 01:03:08Z brobison $
 */

#include <qevent.h>
#include <qmap.h>
#include <qlist.h>
#include <qvariant.h>
#include <qmenu.h>
#include <qlineedit.h>

#include "fieldspinbox.h"

MultipleValueSpinBox::MultipleValueSpinBox( QWidget * parent )
: QSpinBox( parent )
, mOrig( 0 )
, mChanged( false )
, mMultipleValuePaletteSet( false )
{
	init();
}

void MultipleValueSpinBox::init()
{
	connect( this, SIGNAL( valueChanged(int) ), SLOT( slotValueChanged() ) );
	// get a hold of the qspinwidget
	// and install an event filter on it
//	QList<QObject*> = findChildren<QObject*>( "QSpinWidget" );
//	if( l.count() == 1 ){
	//	mSpinWidget = (QSpinWidget*)l[0];
	//	mSpinWidget->installEventFilter( this );
//	}
}

void MultipleValueSpinBox::setValues( QList<int> values )
{
	mOrigValues = values;
	mOrig = 0;
	QMap<int,int> unique;
	foreach( int i, values ) {
		unique[i] = i;
		mOrig = qMax(i,mOrig);
	}

	mUniqueValues = unique.values();
	mChanged = false;
	setValue( mOrig );
	setMultipleValuePalette(mUniqueValues.size() > 1);
}

QList<int> MultipleValueSpinBox::values() const
{
	if( mChanged ) {
		int val = value();
		QList<int> ret;
		for(int i=0; i<mOrigValues.size(); i++ )
			ret.append( val );
		return ret;
	}
	return mOrigValues;
}

QList<int> MultipleValueSpinBox::originalValues() const
{
	return mOrigValues;
}

bool MultipleValueSpinBox::changed() const
{
	return mChanged;
}

void MultipleValueSpinBox::setChanged( bool changed )
{
	if( mChanged != changed ) {
		mChanged = changed;
		setValue( mOrig );
	}
}

void MultipleValueSpinBox::setMultipleValuePalette( bool mvp )
{
	if( mvp != mMultipleValuePaletteSet ) {
		QPalette p;
		if( mvp ) {
			p = palette();
			p.setColor( QPalette::Base, p.color( QPalette::Base ).dark(120) );
		}
		setPalette(p);
		mMultipleValuePaletteSet = mvp;
	}
}

void MultipleValueSpinBox::slotValueChanged()
{
	if( mOrig!=value() && !mChanged ){
		setMultipleValuePalette( false );
		mChanged = true;
	}
	emitValueChanged();
}

QString MultipleValueSpinBox::textFromValue ( int value ) const
{
	if( !mChanged && mUniqueValues.size() > 1 && value == mOrig )
		return "No Change";
	return QSpinBox::textFromValue(value);
}

int MultipleValueSpinBox::valueFromText ( const QString & text ) const
{
	if( !mChanged && text == "No Change" )
		return value();
	return QSpinBox::valueFromText(text);
}

void MultipleValueSpinBox::emitValueChanged()
{
	emit valueChanged( value(), mChanged );
}

void MultipleValueSpinBox::contextMenuEvent( QContextMenuEvent * event )
{
	LOG_5( "FieldSpinBox::customContextMenuEvent: Got context menu event" );
	QPointer<QMenu> menu = lineEdit()->createStandardContextMenu();
	menu->addSeparator();
	const uint se = stepEnabled();
	QAction *up = menu->addAction(tr("&Step up"));
	up->setEnabled(se & StepUpEnabled);
	QAction *down = menu->addAction(tr("Step &down"));
	down->setEnabled(se & StepDownEnabled);
	QAction * noChange = 0;
	QList<const QAction*> valueActions;

	if( mUniqueValues.size() > 1 ) {
		menu->addSeparator();
		noChange = menu->addAction( "No Change" );
		if( !mChanged ) {
			noChange->setCheckable( true );
			noChange->setChecked( true );
		}
		foreach( int v, mUniqueValues ) {
			QString txt = QString::number(v);
			QAction * a = menu->addAction( txt );
			a->setCheckable( true );
			a->setChecked( mChanged && v == value() );
			valueActions.append(a);
		}
	}

	const QPointer<QAbstractSpinBox> that = this;
	const QPoint pos = (event->reason() == QContextMenuEvent::Mouse)
		? event->globalPos() : mapToGlobal(QPoint(event->pos().x(), 0)) + QPoint(width() / 2, height() / 2);
	const QAction *action = menu->exec(pos);
	if (that && action) {
		if (action == up) {
			stepBy(1);
		} else if (action == down) {
			stepBy(-1);
		} else if( action == noChange ) {
			setMultipleValuePalette( mUniqueValues.size() > 1 );
			mChanged = false;
			if( value() == mOrig )
				emitValueChanged();
			setValue( mOrig );
		} else if( valueActions.contains( action ) ) {
			mChanged = true;
			setMultipleValuePalette( false );
			int val = action->text().toInt();
			if( val == value() )
				emitValueChanged();
			setValue( val );
		}
	}
	delete static_cast<QMenu *>(menu);
	event->accept();
}

FieldSpinBox::FieldSpinBox( QWidget * parent )
: MultipleValueSpinBox( parent )
, mProxy( 0 )
{
}

FieldSpinBox::FieldSpinBox( RecordProxy * proxy, const QString & field, QWidget * parent )
: MultipleValueSpinBox( parent )
, mProxy( proxy )
, mField( field )
{
	if( mProxy ){
		slotRecordListChanged();
		connect( mProxy, SIGNAL( recordListChange() ), SLOT( slotRecordListChanged() ) );
		connect( mProxy, SIGNAL( updateRecordList() ), SLOT( slotUpdateRecordList() ) );
	}
}

void FieldSpinBox::setField( const QString & field )
{
	mField = field;
	slotRecordListChanged();
}

QString FieldSpinBox::field() const
{
	return mField;
}

void FieldSpinBox::setProxy( RecordProxy * proxy )
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

void FieldSpinBox::slotRecordListChanged()
{
	if( mProxy && !mField.isEmpty() ){
		RecordList rl = mProxy->records();
		QList<QVariant> vals = rl.getValue( mField );
		QList<int> intVals;
		foreach( QVariant v, vals ) {
			intVals.append( v.toInt() );
		}
		setValues( intVals );
	}
}

void FieldSpinBox::slotUpdateRecordList()
{
	if( changed() && mProxy ){
		mProxy->records().setValue( mField, value() );
	}
}

bool FieldSpinBox::eventFilter( QObject * o, QEvent * e )
{
	/*if( o == mSpinWidget ){
		if( e->type() == QEvent::MouseButtonPress ){
			QMouseEvent * me = (QMouseEvent*)e;
			mStartPos = me->pos().y();
			mStartValue = value();
		}
		else if( e->type() == QEvent::MouseMove ){
			QMouseEvent * me = (QMouseEvent*)e;
			int delta = mStartPos - me->pos().y();
			setValue( mStartValue + delta/2 );
		}
	}*/
	return QSpinBox::eventFilter( o, e );
}

