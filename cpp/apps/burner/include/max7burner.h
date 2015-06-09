
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

#ifdef COMPILE_MAX7_BURNER

#ifndef MAX7_BURNER_H
#define MAX7_BURNER_H

#include "jobburner.h"

/*
 * This class is used to start,
 * and moniter 3dsmax 7 using
 * 3dsmaxcmd.exe
 *
 * Requires Config key 'slaveMax7Dir'
 */

class QTimer;

/// \ingroup ABurner
/// @{

class Max7Burner : public JobBurner
{
Q_OBJECT
public:
	Max7Burner( const JobAssignment & job, Slave * slave );
	~Max7Burner();

	QStringList processNames() const;

	bool checkup();

public slots:
	void startProcess();
	void cleanup();

public slots:
	void slotProcessOutputLine( const QString & line, QProcess::ProcessChannel );
	void slotProcessStarted();

protected:
	QString buildCmd() const;
	QString executable() const;
	QString startupScriptDest();
	QString maxDir() const;

	// Currently used to setup extra exr channels and attributes
	bool generateStartupScript();

	void cleanupTempFiles();

	int mFrame;
	QString mCurrentOutput;
	QString mStartupScriptPath;

	// Store the process id so we can kill the child processes during cleanup
	qint32 mProcessId;
};

/// @}

#endif // MAX7_BURNER_H

#endif
