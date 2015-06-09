
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of Assburner.
 *
 * Assburner is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Assburner is distributed in the hope that it will be useful,
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
 * $LastChangedDate: 2010-03-25 12:18:02 +1100 (Thu, 25 Mar 2010) $
 * $Rev: 9589 $
 * $HeadURL: svn://svn.blur.com/blur/branches/concurrent_burn/cpp/lib/assfreezer/include/settingsdialog.h $
 */

#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <qcolor.h>

#include "ui_settingsdialogui.h"
#include "afcommon.h"
#include "iniconfig.h"
#include "items.h"
#include "blurqt.h"

class FREEZER_EXPORT SettingsDialog : public QDialog, public Ui::SettingsDialogUI
{
Q_OBJECT
public:
	SettingsDialog( QWidget * parent );

signals:

	void apply();
protected slots:

	void slotApply();

	void selectFrameCyclerPath();

	void changes();

protected:

	Options opts;
	bool mChanges;
};


#endif // SETTINGS_DIALOG_H

