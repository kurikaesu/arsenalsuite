 
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
 * $Id: tardstyle.cpp 5411 2007-12-18 01:03:08Z brobison $
 */

#include "tardstyle.h"

#include <qpainter.h>
#include <qstyle.h>
#include <QStyleOption>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qspinbox.h>
#include <qgroupbox.h>
#include <qradiobutton.h>
#include <qsplitter.h>
#include <qtabbar.h>
#include <qgroupbox.h>
#include <qcheckbox.h>

TardStyle::TardStyle()
{
}

TardStyle::~TardStyle()
{
}

static void drawCorner( int x, int y, int xd, int yd, QPainter * p, QColor c )
{
	c.setAlphaF( .25 );
	p->setPen( c );
	p->drawPoint( x+(2*xd),y );
	p->drawPoint( x,y+(2*yd) );
	p->drawPoint( x+(3*xd),y+yd );
	p->drawPoint( x+xd, y+(3*yd) );
	c.setAlphaF( .72 );
	p->setPen( c );
	p->drawPoint( x+(3*xd), y );
	p->drawPoint( x+(2*xd), y+yd);
	p->drawPoint( x+xd, y+(2*yd));
	p->drawPoint( x, y+(3*yd));
	c.setAlphaF( .52 );
	p->setPen( c );
	p->drawPoint( x+xd, y+yd );
}

void TardStyle::polish ( QWidget * widget )
{
	if (qobject_cast<QPushButton *>(widget)
		|| qobject_cast<QComboBox *>(widget)
		|| qobject_cast<QAbstractSpinBox *>(widget)
		|| qobject_cast<QCheckBox *>(widget)
		|| qobject_cast<QGroupBox *>(widget)
		|| qobject_cast<QRadioButton *>(widget)
		|| qobject_cast<QSplitterHandle *>(widget)
		|| qobject_cast<QTabBar *>(widget)
		) {
		widget->setAttribute(Qt::WA_Hover);
	}
	QWindowsStyle::polish(widget);
}

void TardStyle::drawPrimitive ( PrimitiveElement elem, const QStyleOption * option, QPainter * painter, const QWidget * widget ) const
{
	qWarning( "drawPrimitive" );
	if( elem == QStyle::PE_PanelButtonCommand ) {
		if (const QStyleOptionButton *button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
			qWarning( "PE_PanelButtonCommand: State: %i", (int)option->state );
			bool down = (button->state & State_Sunken) || (button->state & State_On);
			bool hover = (button->state & State_Enabled) && (button->state & State_MouseOver);
			bool isDefault = (button->features & QStyleOptionButton::DefaultButton);
			bool isEnabled = (button->state & State_Enabled);
			QColor border(option->palette.background().color().dark(178));
			QColor background(option->palette.button().color());
			if( hover )
				background = background.light(115);
			int x,y,x2,y2;
			option->rect.getCoords(&x,&y,&x2,&y2);
			painter->fillRect( option->rect.adjusted(2,2,-2,-2), background );
			painter->setPen(background);
			painter->drawLine(x+2,y+1,x2-2,y+1);
			painter->drawLine(x+1,y+2,x+1,y2-2);
			painter->drawLine(x+2,y2-1,x2-2,y2-1);
			painter->drawLine(x2-1,y+2,x2-1,y2-2);
			painter->setPen( border ); // Light red
			painter->setBrush( QBrush() );
			int cc=4;
			painter->drawLine( x+cc,y,x2-cc,y );
			painter->drawLine( x+cc,y2,x2-cc,y2 );
			painter->drawLine( x,y+cc,x,y2-cc );
			painter->drawLine( x2,y+cc,x2,y2-cc );
			drawCorner(x,y,1,1,painter,border);
			drawCorner(x2,y,-1,1,painter,border);
			drawCorner(x2,y2,-1,-1,painter,border);
			drawCorner(x,y2,1,-1,painter,border);
			return;
		}
	}
	QWindowsStyle::drawPrimitive(elem,option,painter,widget);
}

void TardStyle::drawControl ( ControlElement element, const QStyleOption * option, QPainter * painter, const QWidget * widget ) const
{
	qWarning( "drawControl" );
/*	if( element == QStyle::CE_PushButton ) {
		qWarning( "CE_PushButton" );
		return;
	} */
	QWindowsStyle::drawControl(element,option,painter,widget);
}

void TardStyle::drawComplexControl ( ComplexControl control, const QStyleOptionComplex * option, QPainter * painter, const QWidget * widget ) const
{
	qWarning( "drawComplexControl" );
	QWindowsStyle::drawComplexControl(control,option,painter,widget);
}


