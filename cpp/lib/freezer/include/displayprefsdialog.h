
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of Arsenal.
 *
 * Arsenal is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Arsenal is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Blur; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* $Author$
 * $LastChangedDate: 2008-04-16 03:17:04 +1000 (Wed, 16 Apr 2008) $
 * $Rev: 6305 $
 * $HeadURL: svn://svn.blur.com/blur/branches/concurrent_burn/cpp/lib/assfreezer/include/displayprefsdialog.h $
 */

#ifndef DISPLAY_PREFS_DIALOG_H
#define DISPLAY_PREFS_DIALOG_H

#include <qcolor.h>

#include "ui_displayprefsdialogui.h"
#include "afcommon.h"
#include "iniconfig.h"
//#include "items.h"
#include "afcommon.h"
#include "blurqt.h"
#include "viewcolors.h"

class FREEZER_EXPORT DisplayPrefsDialog : public QDialog, public Ui::DisplayPrefsDialogUI
{
Q_OBJECT
public:
	DisplayPrefsDialog( QWidget * parent );

signals:

	void apply();
protected slots:

	void slotApply();

	void setApplicationFont();
	void setJobFont();
	void setFrameFont();
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

