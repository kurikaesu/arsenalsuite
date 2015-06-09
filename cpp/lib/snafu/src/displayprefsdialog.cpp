
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of Snafu.
 *
 * Snafu is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Snafu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Snafu; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id: displayprefsdialog.cpp 5408 2007-12-18 00:13:49Z brobison $
 */

#include <qpushbutton.h>
#include <qpainter.h>
#include <qcolordialog.h>
#include <qfontdialog.h>
#include <qlineedit.h>
#include <qmenu.h>

#include "displayprefsdialog.h"

void applyColor( QPushButton *, const QColor & );

static int ColorItemType = QTreeWidgetItem::UserType;

class ColorItem : public QTreeWidgetItem
{
public:
	ColorOption mReset;
	ColorOption * mColorOption;
	ViewColors * mViewColors;
	QColor fg, bg;

	ColorItem( ColorOption * co, ViewColors * vc, QTreeWidgetItem * parent )
	: QTreeWidgetItem( parent, ColorItemType )
	, mColorOption( co )
	, mViewColors( vc )
	{
		mReset = *co;
		fg = mReset.fg;
		bg = mReset.bg;
		update();
	}
	
	ColorItem( ColorOption * co, ViewColors * vc, QTreeWidget * parent )
	: QTreeWidgetItem( parent, ColorItemType )
	, mColorOption( co )
	, mViewColors( vc )
	{
		mReset = *co;
		fg = mReset.fg;
		bg = mReset.bg;
		update();
	}

	void update()
	{
		setText( 0, mReset.role );
		setBackgroundColor(1,fg);
		setBackgroundColor(2,bg);
	}
	void apply()
	{
		mColorOption->fg = fg;
		mColorOption->bg = bg;
		mViewColors->mTreeView->update();
	}
	void reset()
	{
		*mColorOption = mReset;
		mViewColors->mTreeView->update();
	}
};

void ViewColors::readColors()
{
	IniConfig & c = userConfig();
	c.pushSection( mViewName );
	for( int i=0; i<mColors.size(); i++ )
	{
		ColorOption & co = mColors[i];
		co.fg = c.readColor( co.role + "fg", co.fg );
		co.bg = c.readColor( co.role + "bg", co.bg );
	}
	c.popSection();
}

void ViewColors::writeColors()
{
	IniConfig & c = userConfig();
	c.pushSection( mViewName );
	for( int i=0; i<mColors.size(); i++ )
	{
		ColorOption & co = mColors[i];
		c.writeColor( co.role + "fg", co.fg );
		c.writeColor( co.role + "bg", co.bg );
	}
	c.popSection();
}

void ViewColors::apply()
{
	ColorOption * c = getColorOption("Default");
	if( c ) {
		QPalette p = mTreeView->palette();
		p.setColor( QPalette::Base, c->bg );
		p.setColor( QPalette::Text, c->fg );
		mTreeView->setPalette(p);
	}
}

void ViewColors::setupView( QTreeWidget * view )
{
	ColorItem * p = new ColorItem( getColorOption("Default"), this, view );
	p->setText( 0, mViewName );
	for( int i=0; i<mColors.size(); i++ )
	{
		if( mColors[i].role == "Default" ) continue;
		new ColorItem( &mColors[i], this, p );
	}
	view->setItemExpanded(p,true);
}

ColorOption * ViewColors::getColorOption( const QString & name )
{
	QString n = name.toLower();
	for( int i=0; i<mColors.size(); i++ )
		if( mColors[i].role.toLower() == n )
			return &mColors[i];
	return 0;
}

void ViewColors::getColors( const QString & name, QColor & fg, QColor & bg )
{
	QString n = name.toLower();
	for( int i=0; i<mColors.size(); i++ )
		if( mColors[i].role.toLower() == n ) {
			fg = mColors[i].fg;
			bg = mColors[i].bg;
			return;
		}
}

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

	SummaryFontEdit->setText( opts.summaryFont.toString() );

	connect( SummaryFontEdit, SIGNAL( textChanged( const QString & ) ), SLOT( changes() ) );

	connect( SummaryFontButton, SIGNAL( clicked() ), SLOT( setSummaryFont() ) );

	mColorTree->setHeaderLabels( QStringList() << "Role" << "Text" << "Background" );
	mColorTree->setContextMenuPolicy( Qt::CustomContextMenu );
	mColorTree->setSelectionMode(QAbstractItemView::NoSelection);
	
	options.mHostColors->setupView( mColorTree );

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
		opts.summaryFont.fromString( SummaryFontEdit->text() );
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

