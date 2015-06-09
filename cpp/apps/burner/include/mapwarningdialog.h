
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
 * along with Arsenal; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id$
 */

#ifndef MAP_WARNING_DIALOG_H
#define MAP_WARNING_DIALOG_H

#include <qdialog.h>

#include "ui_mapwarningdialogui.h"

/// \ingroup ABurner
/// @{

class TimedDialog : public QDialog, public Ui::MapWarningDialogUI
{
public:
	TimedDialog( QWidget * parent, const QString & title, const QString & message, const QString & dontShowKey, int timeout = 30 );

	void accept();

protected:
	QString mConfigKey;
};

class MapWarningDialog : public TimedDialog
{
public:
	MapWarningDialog( QWidget * parent = 0 )
	: TimedDialog(
		parent,
		"Warning, Assburner is about to remap your network drives",
		"If you leave any files from the network open while Assburner has your "
		"workstation in render mode, those open files may become disconnected and"
		" crash applications that were left open. When you \"Stop the Burn\", Assburner"
		" will attempt to return your G: mapping to the real fileserver.",
		"WarnBeforeMapping"
	) {}
};

class ReMapWarningDialog : public TimedDialog
{
public:
	ReMapWarningDialog( QWidget * parent = 0 )
	: TimedDialog(
		parent, 
		"Warning, Assburner is about to remap your network drives",
		"Assburner is restoring your default fileserver mappings after rendering. "
		"Leaving network files open while rendering may cause application crashes.",
		"WarnBeforeReMapping"
	) {
		mCancelButton->hide();
	}
};

/// @}

#endif // MAP_WARNING_DIALOG_H

