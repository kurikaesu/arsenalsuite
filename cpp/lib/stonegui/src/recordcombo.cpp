
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
 * $Id: recordcombo.cpp 6486 2008-05-02 01:34:01Z newellm $
 */

#include <qstylepainter.h>
#include <qtooltip.h>

#include "recordcombo.h"

#include "record.h"
#include "recordlistmodel.h"
#include "field.h"
#include "index.h"
#include "table.h"
#include "tableschema.h"

RecordCombo::RecordCombo( QWidget * parent )
: QComboBox( parent )
, mTable( 0 )
, mIndex( 0 )
{
	setModel( new RecordListModel( this ) );
	connect( this, SIGNAL( currentIndexChanged( int ) ), SLOT( slotCurrentChanged( int ) ) );
	connect( this, SIGNAL( highlighted( int ) ), SLOT( slotHighlighted( int ) ) );
}

void RecordCombo::setTable( Table * table )
{
	mTable = table;
	refresh( true );
}

void RecordCombo::setColumn( const QString & column )
{
	mColumn = column;
	if( mModel->inherits( "RecordListModel" ) ) {
		RecordListModel* rlm = (RecordListModel*)mModel;
		rlm->setColumn( column );
	}
	refresh( true );
}

void RecordCombo::setItems( RecordList rl )
{
	mModel->setRootList( rl );
	setCurrentIndex( 0 );
}

void RecordCombo::refresh( bool checkIndex )
{
	if( mTable && !mColumn.isEmpty() )
	{
		RecordList rl = mTable->select();
		mModel->setRootList( rl );
		mModel->sort( 0, Qt::DescendingOrder );
		if( checkIndex ) {
			Field * f = mTable->schema()->field( mColumn );
			mIndex = mTable->indexFromSchema( f->index() );
		}
	}
}

void RecordCombo::setModel( RecordSuperModel * model )
{
	mModel = model;
	QComboBox::setModel( model );
}

void RecordCombo::setCurrent( const Record & r )
{
	QModelIndex i = mModel->findIndex( r );
	if( i.isValid() )
		setCurrentIndex( i.row() );
}

void RecordCombo::slotCurrentChanged( int row )
{
	QModelIndex i = mModel->index(row,0);
	Record r;
	if( i.isValid() ) {
		QVariant v = mModel->data(i,Qt::ToolTipRole);
		if( v.isValid() && v.type() == QVariant::String )
			QMetaObject::invokeMethod( this, "showTip", Qt::QueuedConnection, Q_ARG(QString,v.toString()) );
		r = mModel->getRecord(i);
	}
	emit currentChanged( r );
}

void RecordCombo::slotHighlighted( int row )
{
	QModelIndex i = mModel->index(row,0);
	Record r;
	if( i.isValid() ) {
		QVariant v = mModel->data(i,Qt::ToolTipRole);
		if( v.isValid() && v.type() == QVariant::String )
			showTip( v.toString() );
		r = mModel->getRecord(i);
	}
	emit highlighted( r );
}

void RecordCombo::showTip( const QString & text )
{
	QToolTip::showText( mapToGlobal( QPoint(width(),height()) ), text, this );
}

void RecordCombo::paintEvent( QPaintEvent * pe )
{
	QComboBox::paintEvent( pe );
	QVariant v = itemData( currentIndex(), Qt::BackgroundColorRole );
	if( v.isValid() && v.type() == QVariant::Color ) {
		QColor c( qvariant_cast<QColor>(v) );
		
		QStyleOptionComboBox opt;
		opt.init( this );
		opt.editable = isEditable();
		opt.frame = hasFrame();
		if (currentIndex() >= 0) {
			opt.currentText = currentText();
		}
	
		opt.palette.setColor( QPalette::Background, c );
		opt.palette.setColor( QPalette::Text, c.value() < 128 ? Qt::white : Qt::black );
		{
			QStylePainter painter(this);
			QRect editRect = style()->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxEditField, this);
			painter.fillRect( editRect, c );
			painter.setPen( c.value() < 128 ? Qt::white : Qt::black );
			painter.drawControl(QStyle::CE_ComboBoxLabel, opt);
		}
	}
}

