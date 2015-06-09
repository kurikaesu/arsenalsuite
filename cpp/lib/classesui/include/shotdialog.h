/*
 *
 * Copyright 2003, 2004 Blur Studio Inc.
 *
 * This file is part of the Resin software package.
 *
 * Resin is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Resin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Resin; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef SHOT_DIALOG_H
#define SHOT_DIALOG_H

#include "classesui.h"

#include "ui_shotdialogui.h"

#include "assettemplate.h"
#include "element.h"
#include "shot.h"
#include "thumbnail.h"

class TasksWidget;

class CLASSESUI_EXPORT ShotDialog : public QDialog, public Ui::ShotDialogUI
{
Q_OBJECT
public:
	ShotDialog( const Element & element, QWidget * parent=0 );

	void setShotNumber( float );

	void setShotName( const QString & name );
	
	 /// Returns an uncommitted shot record matching the current properties of the form
	Shot shotSetup();

	 /// Returns the list of actual shots committed to the database when the user
	 /// pressed OK.  Returns an empty list if there were no shots created.
	ShotList createdShots() const;

	virtual void accept();
	
public slots:
	void updateResult();
	void shotStartChange( const QString & value );
	void shotEndChange( const QString & value );

protected:
	TasksWidget * mTasksWidget;
	Element mElement;
	ShotList mCreated;
};

#endif // SHOT_DIALOG_H

