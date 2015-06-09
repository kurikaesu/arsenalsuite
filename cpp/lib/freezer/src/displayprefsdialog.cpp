
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of Arsenal.
 *
 * Arsenal is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Arsenal is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Blur; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* $Author$
 * $LastChangedDate: 2008-04-16 03:17:04 +1000 (Wed, 16 Apr 2008) $
 * $Rev: 6305 $
 * $HeadURL: svn://svn.blur.com/blur/branches/concurrent_burn/cpp/lib/assfreezer/src/displayprefsdialog.cpp $
 */

#include <qpushbutton.h>
#include <qpainter.h>
#include <qcolordialog.h>
#include <qfontdialog.h>
#include <qlineedit.h>
#include <qmenu.h>

#include "displayprefsdialog.h"

void applyColor( QPushButton *, const QColor & );


Options options;

DisplayPrefsDialog::DisplayPrefsDialog( QWidget * parent )
: QDialog( parent )
, mChanges( false )
{
	setupUi( this );

	connect( ApplyButton, SIGNAL( clicked() ), SLOT( slotApply() ) );
	connect( OKButton, SIGNAL( clicked() ), SLOT( slotApply() ) );
	opts = options;

	//
	// Look & Feel Tab
	//

	ApplicationFontEdit->setText( opts.appFont.toString() );
	JobFontEdit->setText( opts.jobFont.toString() );
	FrameFontEdit->setText( opts.frameFont.toString() );
	SummaryFontEdit->setText( opts.summaryFont.toString() );

	connect( ApplicationFontEdit, SIGNAL( textChanged( const QString & ) ), SLOT( changes() ) );
	connect( JobFontEdit, SIGNAL( textChanged( const QString & ) ), SLOT( changes() ) );
	connect( FrameFontEdit, SIGNAL( textChanged( const QString & ) ), SLOT( changes() ) );
	connect( SummaryFontEdit, SIGNAL( textChanged( const QString & ) ), SLOT( changes() ) );

	connect( ApplicationFontButton, SIGNAL( clicked() ), SLOT( setApplicationFont() ) );
	connect( JobFontButton, SIGNAL( clicked() ), SLOT( setJobFont() ) );
	connect( FrameFontButton, SIGNAL( clicked() ), SLOT( setFrameFont() ) );
	connect( SummaryFontButton, SIGNAL( clicked() ), SLOT( setSummaryFont() ) );

	mColorTree->setHeaderLabels( QStringList() << "Role" << "Text" << "Background" );
	mColorTree->setContextMenuPolicy( Qt::CustomContextMenu );
	mColorTree->setSelectionMode(QAbstractItemView::NoSelection);
	
	options.mJobColors->setupView( mColorTree );
	options.mHostColors->setupView( mColorTree );
	options.mFrameColors->setupView( mColorTree );
	options.mErrorColors->setupView( mColorTree );

	connect( mColorTree, SIGNAL( itemActivated( QTreeWidgetItem *, int ) ), SLOT( colorItemActivated( QTreeWidgetItem*, int ) ) );
	connect( mColorTree, SIGNAL( customContextMenuRequested(const QPoint &) ), SLOT( colorItemMenu( const QPoint & ) ) );
	ApplyButton->setEnabled( false );
}
/*
void DisplayPrefsDialog::setColor( QPushButton * button, QColor & color )
{
	color = QColorDialog::getColor( color, this );
	if( color.isValid() ){
		applyColor( button, color );
		changes();
	}
}
*/
void DisplayPrefsDialog::updateColor( QColor & color )
{
	QColor newcolor = QColorDialog::getColor( color, this );
	if( newcolor.isValid() ) {
		color = newcolor;
		ApplyButton->setEnabled( true );
		mChanges = true;
	}
}

void DisplayPrefsDialog::colorItemActivated( QTreeWidgetItem * item, int column )
{
	if( item->type() == ColorItemType && column > 0 ) {
		ColorItem * c = static_cast<ColorItem*>(item);
		if( column == 1 )
			updateColor( c->fg );
		else if( column == 2 )
			updateColor( c->bg );
		c->update();
		//mColorTree->update();
	}
}

void DisplayPrefsDialog::colorItemMenu( const QPoint & pos )
{
	QTreeWidgetItem * c = mColorTree->currentItem();
	int cc = mColorTree->currentColumn();
	if( c && cc > 0 && c->type() == ColorItemType ) {
		ColorItem * ci = static_cast<ColorItem*>(c);
		QMenu * m = new QMenu(this);
		QAction * c = m->addAction( "Clear" );
		QAction * r = m->exec( mColorTree->mapToGlobal(pos) );
		if( r==c ) {
			if( cc == 1 )
				ci->fg = QColor();
			else if( cc == 2 )
				ci->bg = QColor();
			ci->update();
			mChanges = true;
			ApplyButton->setEnabled(true);
		}
		delete m;
	}
}

void DisplayPrefsDialog::setApplicationFont()
{
	bool ok;
	QFont tf = QFontDialog::getFont( &ok, opts.appFont, this );
	if( ok ){
		opts.appFont = tf;
		ApplicationFontEdit->setText( tf.toString() );
	}
}

void DisplayPrefsDialog::setJobFont()
{
	bool ok;
	QFont tf = QFontDialog::getFont( &ok, opts.jobFont, this );
	if( ok ){
		opts.jobFont = tf;
		JobFontEdit->setText( tf.toString() );
	}
}

void DisplayPrefsDialog::setFrameFont()
{
	bool ok;
	QFont tf = QFontDialog::getFont( &ok, opts.frameFont, this );
	if( ok ){
		opts.frameFont = tf;
		FrameFontEdit->setText( tf.toString() );
	}
}

void DisplayPrefsDialog::setSummaryFont()
{
	bool ok;
	QFont tf = QFontDialog::getFont( &ok, opts.summaryFont, this );
	if( ok ){
		opts.summaryFont = tf;
		SummaryFontEdit->setText( tf.toString() );
	}
}

void DisplayPrefsDialog::slotApply()
{
	if( mChanges ){
		for( QTreeWidgetItemIterator it( mColorTree ); *it; ++it )
			if( (*it)->type() == ColorItemType ) {
				static_cast<ColorItem*>(*it)->apply();
			}
		opts.appFont.fromString( ApplicationFontEdit->text() );
		opts.summaryFont.fromString( SummaryFontEdit->text() );
		opts.frameFont.fromString( FrameFontEdit->text() );
		opts.jobFont.fromString( JobFontEdit->text() );
		options = opts;
		mChanges = false;
		ApplyButton->setEnabled( false );
		emit apply();
	}
}

void DisplayPrefsDialog::changes()
{
	mChanges = true;
	ApplyButton->setEnabled( true );
}

