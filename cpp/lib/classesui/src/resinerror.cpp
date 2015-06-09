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

#include <stdlib.h>

#include <qmessagebox.h>
#include <qpixmap.h>

#include "resinerror.h"

void ResinError::nameEmpty( QWidget * parent, const QString & name )
{
	bool usAn = QString("aeiou").contains( name.left(1) );
	QMessageBox::warning( 
		parent, 
		name + " is empty", 
		"Please enter a" + QString(usAn?"n ":" ") + name, 
		QMessageBox::Ok, 
		QMessageBox::NoButton 
	);
}

void ResinError::nameTaken( QWidget * parent, const QString & name )
{
	QMessageBox::warning( 
		parent, 
		name + " is already used", 
		name + " is already used.\nPlease enter a different name.", 
		QMessageBox::Ok, 
		QMessageBox::NoButton 
	);
}

bool ResinError::deleteConfirmation( QWidget * parent )
{
	QMessageBox mb("Delete Confirmation",
		"Deleting this is permanent(unless you hit undo before closing resin), are you sure?",
		QMessageBox::Critical,
		QMessageBox::Yes,
		QMessageBox::No | QMessageBox::Default,
		QMessageBox::NoButton,
		parent );
	return mb.exec() == QMessageBox::Yes;
}

bool ResinError::renameCausesMoveWarning( QWidget * parent )
{
	QMessageBox mb( "Rename Confirmation - Potential Breakage",
		"Renaming this element will cause the files and/or folders to be renamed. Doing this can "
		"potentially break files that contain XRefs, etc. Are you sure that you want to continue?",
		QMessageBox::Critical,
		QMessageBox::Yes, 
		QMessageBox::No | QMessageBox::Default,
		QMessageBox::NoButton,
		parent );
	return mb.exec() == QMessageBox::Yes;
}

