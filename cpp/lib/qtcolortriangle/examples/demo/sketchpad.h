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
#ifndef SKETCHPAD_H
#define SKETCHPAD_H
#include <qwidget.h>
#include <qpoint.h>
#include <qcolor.h>
#include <qpixmap.h>
//Added by qt3to4:
#include <QMouseEvent>
#include <QResizeEvent>
#include <QPaintEvent>

class QMouseEvent;
class QResizeEvent;
class QPaintEvent;

class SketchPad : public QWidget
{
    Q_OBJECT
public:
    SketchPad(QWidget *parent = 0);
    
    QColor color() const;

public slots:
    void setColor(const QColor &color);
    void clear();
    
protected:
    void mouseMoveEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void resizeEvent(QResizeEvent *e);
    void paintEvent(QPaintEvent *e);

private:
    QPoint lastPos;
    QColor c;
    QPixmap pix;
};

#endif
