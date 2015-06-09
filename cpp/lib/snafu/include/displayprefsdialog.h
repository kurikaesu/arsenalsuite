
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
 * $Id: displayprefsdialog.h 5408 2007-12-18 00:13:49Z brobison $
 */

#ifndef SNAFU_DISPLAY_PREFS_DIALOG_H
#define SNAFU_DISPLAY_PREFS_DIALOG_H

#include <qcolor.h>

#include "ui_displayprefsdialogui.h"
#include "iniconfig.h"
#include "blurqt.h"

class QTreeWidget;

struct ColorOption
{
	ColorOption( const QString & r=QString(), QColor f=QColor(), QColor b=QColor() ) : fg(f), bg(b), role(r) {}
	QColor fg, bg;
	QString role;
};

struct ViewColors
{
	ViewColors( const QString & viewname, QTreeView * view )
	: mViewName( viewname ), mTreeView( view ) {}
	QList<ColorOption> mColors;
	QString mViewName;
	QTreeView * mTreeView;
	void readColors();
	void writeColors();
	void apply();
	void setupView( QTreeWidget * );
	ColorOption * getColorOption( const QString & name );
	void getColors( const QString & name, QColor & fg, QColor & bg );
};

struct Options
{
  Options() : mHostColors(0) {}
  ViewColors * mHostColors;
  QFont summaryFont;
  int mLimit;
};

class DisplayPrefsDialog : public QDialog, public Ui::DisplayPrefsDialogUI
{
Q_OBJECT
public:
	DisplayPrefsDialog( QWidget * parent );

signals:

	void apply();
protected slots:

	void slotApply();

	void setSummaryFont();

	void changes();

	void colorItemActivated( QTreeWidgetItem * item, int column );
	void colorItemMenu( const QPoint & );

protected:
    void updateColor( QColor & color );

	Options opts;
	bool mChanges;
};


#endif // DISPLAY_PREFS_DIALOG_H

