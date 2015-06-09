
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
 * $Id: viewcolors.h 5993 2008-03-27 22:30:35Z newellm $
 */

#ifndef VIEW_COLORS_H
#define VIEW_COLORS_H

#include <qcolor.h>
#include <qtreewidget.h>

#include "stonegui.h"

struct STONEGUI_EXPORT ColorOption
{
	ColorOption( const QString & r=QString(), QColor f=QColor(), QColor b=QColor() ) : fg(f), bg(b), role(r) {}
	QColor fg, bg;
	QString role;
};

struct STONEGUI_EXPORT ViewColors
{
	ViewColors( const QString & viewname )
	: mViewName( viewname ) {}
	QList<ColorOption> mColors;
	QString mViewName;
	void readColors();
	void writeColors();
	void apply( QTreeView * );
	void setupView( QTreeWidget * );
	ColorOption * getColorOption( const QString & name );
	void getColors( const QString & name, QColor & fg, QColor & bg );
};

#define ColorItemType (QTreeWidgetItem::UserType)

class STONEGUI_EXPORT ColorItem : public QTreeWidgetItem
{
public:
	ColorOption mReset;
	ColorOption * mColorOption;
	ViewColors * mViewColors;
	QColor fg, bg;

	ColorItem( ColorOption * co, ViewColors * vc, QTreeWidgetItem * parent );
	
	ColorItem( ColorOption * co, ViewColors * vc, QTreeWidget * parent );

	void update();
	void apply();
	void reset();
};

#endif // VIEW_COLORS_H

