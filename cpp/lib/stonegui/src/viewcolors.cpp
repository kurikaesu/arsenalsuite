
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

#include <qpalette.h>
#include <qtreeview.h>

#include "iniconfig.h"

#include "viewcolors.h"

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

void ViewColors::apply( QTreeView * treeView )
{
	ColorOption * c = getColorOption("default");
	if( c ) {
		QPalette p = treeView->palette();
		p.setColor( QPalette::Base, c->bg );
		p.setColor( QPalette::Text, c->fg );
		treeView->setPalette(p);
	}
}

void ViewColors::setupView( QTreeWidget * view )
{
	ColorItem * p = new ColorItem( getColorOption("default"), this, view );
	p->setText( 0, mViewName );
	for( int i=0; i<mColors.size(); i++ )
	{
		if( mColors[i].role == "default" ) continue;
		new ColorItem( &mColors[i], this, p );
	}
	view->setItemExpanded(p,true);
}

ColorOption * ViewColors::getColorOption( const QString & name )
{
	for( int i=0; i<mColors.size(); i++ )
		if( mColors[i].role == name )
			return &mColors[i];
	return 0;
}

void ViewColors::getColors( const QString & name, QColor & fg, QColor & bg )
{
	for( int i=0; i<mColors.size(); i++ )
		if( mColors[i].role == name ) {
			fg = mColors[i].fg;
			bg = mColors[i].bg;
			return;
		}
}

ColorItem::ColorItem( ColorOption * co, ViewColors * vc, QTreeWidgetItem * parent )
: QTreeWidgetItem( parent, ColorItemType )
, mColorOption( co )
, mViewColors( vc )
{
	mReset = *co;
	fg = mReset.fg;
	bg = mReset.bg;
	update();
}

ColorItem::ColorItem( ColorOption * co, ViewColors * vc, QTreeWidget * parent )
: QTreeWidgetItem( parent, ColorItemType )
, mColorOption( co )
, mViewColors( vc )
{
	mReset = *co;
	fg = mReset.fg;
	bg = mReset.bg;
	update();
}

void ColorItem::update()
{
	setText( 0, mReset.role );
	setBackgroundColor(1,fg);
	setBackgroundColor(2,bg);
}

void ColorItem::apply()
{
	mColorOption->fg = fg;
	mColorOption->bg = bg;
	//mViewColors->mTreeView->update();
}

void ColorItem::reset()
{
	*mColorOption = mReset;
	//mViewColors->mTreeView->update();
}

