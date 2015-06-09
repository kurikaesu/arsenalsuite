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
 * $Id: tailwindow.h 5411 2007-12-18 01:03:08Z brobison $
 */

#ifndef TAILWINDOW_H
#define TAILWINDOW_H

#include <QFile>
#include <QTimer>
#include <QTextStream>

#include "ui_tailwindow.h"

class TailWindow : public QWidget
{
    Q_OBJECT

public:
    TailWindow(QWidget *parent = 0);
    ~TailWindow();

public slots:
    void stop(void);
    void start(void);

private slots:
    void textChanged(void);
    void updateText(void);
    void toggleStartStop(bool isToggled);
    void setBufferSizeLabelValue(int size);

private:
    Ui::TailWindow ui;
  
protected:

    qint64 mTextSize;
    QFile mFile;
    QTimer * mTimer;
    QScrollBar * mScrollBar;
    bool atScrollEnd;

    QTextStream * mLogStream;
    QString mReadBuffer;
    qint64 mAmountRead;
};

#endif

