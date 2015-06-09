
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

#ifndef KILL_DIALOG_H
#define KILL_DIALOG_H

#include "ui_killdialogui.h"

class QTimer;

/// \ingroup ABurner
/// @{

class KillDialog : public QDialog, public Ui::KillDialogUI
{
Q_OBJECT
public:
	KillDialog( QStringList, QWidget * parent=0 );

public slots:
	void refresh();
	void kill();

protected:
	QTimer * mRefreshTimer;
	QStringList mProcessNames;
	QMap<QString, QList<int> > mPidMap;

};

/// @}

#endif // KILL_DIALOG_H

