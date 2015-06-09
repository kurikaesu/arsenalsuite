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

#ifndef VERSION_TRACKER_DIALOG_H
#define VERSION_TRACKER_DIALOG_H
 
#include "classesui.h"
 
#include "ui_versiontrackerdialogui.h"
#include "versionfiletracker.h"

class CLASSESUI_EXPORT VersionTrackerDialog : public QDialog, public Ui::VersionTrackerDialogUI
{
Q_OBJECT
public:
	VersionTrackerDialog( QWidget * parent );
	
	VersionFileTracker tracker() const;
	void setTracker( const VersionFileTracker & );
	
	virtual void accept();
protected:

	mutable VersionFileTracker mTracker;
};
 
#endif // VERSION_TRACKER_DIALOG_H

