/****************************************************************************
**
** Copyright (C) 2003-2005 Trolltech AS. All rights reserved.
**
** This file is part of a Qt Solutions component.
**
** Licensees holding valid Qt Solutions licenses may use this file in
** accordance with the Qt Solutions License Agreement provided with the
** Software.
**
** See http://www.trolltech.com/products/solutions/index.html 
** or email sales@trolltech.com for information about Qt Solutions
** License Agreements.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/
#include "sketchpad.h"
#include "palm.xpm"

#include <qwidget.h>
#include <qpixmap.h>
#include <qpainter.h>
#include <qcursor.h>
#include <qevent.h>
#include <qlabel.h>
#include <qmessagebox.h>
//Added by qt3to4:
#include <QMouseEvent>
#include <QResizeEvent>
#include <QPaintEvent>

#include <stdlib.h>

/*!
    Constructs a sketch pad. This is simply a widget that we can draw
    on. Makes the pad white.
*/
SketchPad::SketchPad(QWidget *parent)
    : QWidget(parent), pix(size())
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setCursor(QCursor(Qt::CrossCursor));

    pix = QPixmap((const char **)palm_xpm);
}

/*!
    Sets the color of the pen that draws on the sketch pad to be \a
    color.
*/
void SketchPad::setColor(const QColor &color)
{
    c = color;
}

/*!
    Clears the 
*/
void SketchPad::clear()
{
    pix.fill();
    update();
}

/*!
    Returns the current color of the pen on the sketch pad.
*/
QColor SketchPad::color() const
{
    return c;
}

/*! \reimp
*/
void SketchPad::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);

    QPixmap copy(pix);
    pix = QPixmap(size());
    pix.fill(Qt::white);
    QPainter p(&pix);
    p.drawPixmap(QPoint(0, 0), copy);
    update();
}

/*! \reimp

    Draws a line from the previous drawn point to the current mouse
    cursor position.
*/
void SketchPad::mouseMoveEvent(QMouseEvent *e)
{
    QPainter p(&pix);
    p.setPen(QPen(c, 4));
    for (int i = 0; i < 30; ++i) {
	p.drawPoint(e->pos().x() - rand() % 5 + rand() % 5,
		    e->pos().y() - rand() % 5 + rand() % 5);
    }
    lastPos = e->pos();
    update();
}

/*! \reimp

    Draws a point where the mouse cursor is at currently.
*/
void SketchPad::mousePressEvent(QMouseEvent *e)
{
    QPainter p(&pix);
    p.setPen(QPen(c, 2));
    for (int i = 0; i < 30; ++i) {
	p.drawPoint(e->pos().x() - rand() % 5 + rand() % 5,
		    e->pos().y() - rand() % 5 + rand() % 5);
    }
    lastPos = e->pos();
    update();
}

/*! \reimp
*/
void SketchPad::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.drawPixmap(QPoint(0, 0), pix);
}
