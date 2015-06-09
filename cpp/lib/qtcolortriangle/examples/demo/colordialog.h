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
#ifndef COLORDIALOG_H
#define COLORDIALOG_H
#include <qdialog.h>

class SketchPad;
class QLCDNumber;
class QtColorTriangle;
class QColor;
class QSlider;

class ColorDialog : public QWidget
{
    Q_OBJECT
public:
    ColorDialog(QWidget *parent = 0);
    ~ColorDialog();

public slots:
    void setColor(const QColor &c);

    void setRed(int);
    void setGreen(int);
    void setBlue(int);

private:
    QSlider *spRed;
    QSlider *spGreen;
    QSlider *spBlue;

    QLCDNumber *lcdRed;
    QLCDNumber *lcdGreen;
    QLCDNumber *lcdBlue;

    QtColorTriangle *triangle;

    SketchPad *pad;
};

#endif
