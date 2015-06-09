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

#ifndef RESOLUTION_DIALOG_H
#define RESOLUTION_DIALOG_H

#include "classesui.h"

#include "projectresolution.h"
#include "ui_resolutiondialogui.h"

class CLASSESUI_EXPORT ResolutionDialog : public QDialog, public Ui::ResolutionDialogUI
{
Q_OBJECT
public:
	ResolutionDialog( QWidget * parent );

	ProjectResolution resolution();
	void setResolution( const ProjectResolution & );

	virtual void accept();

public slots:
	void setFillFrameImage();
	
protected:
	ProjectResolution mResolution;

};

#endif //  RESOLUTION_DIALOG_H

