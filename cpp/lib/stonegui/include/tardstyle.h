
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
 * $Id: tardstyle.h 5411 2007-12-18 01:03:08Z brobison $
 */

#ifndef TARDSTYLE_H
#define TARDSTYLE_H

#include "qwindowsstyle.h"

class TardStyle : public QWindowsStyle
{
    Q_OBJECT
public:
    TardStyle();
    ~TardStyle();

	virtual void drawControl ( ControlElement element, const QStyleOption * option, QPainter * painter, const QWidget * widget = 0 ) const;
	virtual void drawPrimitive ( PrimitiveElement elem, const QStyleOption * option, QPainter * painter, const QWidget * widget = 0 ) const;
	virtual void drawComplexControl ( ComplexControl control, const QStyleOptionComplex * option, QPainter * painter, const QWidget * widget = 0 ) const;
	virtual void polish ( QWidget * widget );

/*
	virtual void drawItemPixmap ( QPainter * painter, const QRect & rect, int alignment, const QPixmap & pixmap ) const 
	virtual void drawItemText ( QPainter * painter, const QRect & rect, int alignment, const QPalette & pal, bool enabled, const QString & text, QPalette::ColorRole textRole = QPalette::NoRole ) const 
	virtual QPixmap generatedIconPixmap ( QIcon::Mode iconMode, const QPixmap & pixmap, const QStyleOption * option ) const = 0 
	virtual SubControl hitTestComplexControl ( ComplexControl control, const QStyleOptionComplex * option, const QPoint & pos, const QWidget * widget = 0 ) const = 0 
	virtual QRect itemPixmapRect ( const QRect & rect, int alignment, const QPixmap & pixmap ) const 
	virtual QRect itemTextRect ( const QFontMetrics & metrics, const QRect & rect, int alignment, bool enabled, const QString & text ) const 
	virtual int pixelMetric ( PixelMetric metric, const QStyleOption * option = 0, const QWidget * widget = 0 ) const = 0 
	virtual void polish ( QApplication * app ) 
	virtual void polish ( QPalette & pal ) 
	virtual QSize sizeFromContents ( ContentsType type, const QStyleOption * option, const QSize & contentsSize, const QWidget * widget = 0 ) const = 0 
	QIcon standardIcon ( StandardPixmap standardIcon, const QStyleOption * option = 0, const QWidget * widget = 0 ) const 
	virtual QPalette standardPalette () const 
	virtual QPixmap standardPixmap ( StandardPixmap standardPixmap, const QStyleOption * option = 0, const QWidget * widget = 0 ) const = 0 
	virtual int styleHint ( StyleHint hint, const QStyleOption * option = 0, const QWidget * widget = 0, QStyleHintReturn * returnData = 0 ) const = 0 
	virtual QRect subControlRect ( ComplexControl control, const QStyleOptionComplex * option, SubControl subControl, const QWidget * widget = 0 ) const = 0 
	virtual QRect subElementRect ( SubElement element, const QStyleOption * option, const QWidget * widget = 0 ) const = 0 
	virtual void unpolish ( QWidget * widget ) 
	virtual void unpolish ( QApplication * app )
*/

private:
    // Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    TardStyle( const TardStyle & );
    TardStyle& operator=( const TardStyle & );
#endif
};

#endif // QWINDOWSSTYLE_H

