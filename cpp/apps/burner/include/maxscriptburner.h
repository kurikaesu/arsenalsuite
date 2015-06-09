
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

#ifdef COMPILE_MAXSCRIPT_BURNER

#ifndef MAX_SCRIPT_BURNER_H
#define MAX_SCRIPT_BURNER_H


#include "jobburner.h"

class QProcess;

/// \ingroup ABurner
/// @{

/**
 * This burner is used to run
 * 3dsmax.exe with a maxscript.
 * It communicates via file.
 * Tested with Max7 and Max8
 */
class MaxScriptBurner : public JobBurner
{
Q_OBJECT
public:
	MaxScriptBurner( const JobAssignment & jobAssignment, Slave * slave );
	~MaxScriptBurner();

	virtual QStringList processNames() const;
	virtual bool checkup();

public slots:
	void startProcess();
	void cleanup();

protected:
	QString runScriptDest();
	QString maxDir();

	int mStatusLine;
	QString mRunScriptDest;
	QString mStatusFile;
};

/// @}

#endif // MAX_SCRIPT_BURNER_H

#endif

