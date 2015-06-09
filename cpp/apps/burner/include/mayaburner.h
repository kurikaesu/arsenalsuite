
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

#ifdef COMPILE_MAYA_BURNER

#ifndef MAYA_BURNER_H
#define MAYA_BURNER_H

#include "jobburner.h"

/*
 * This class is used to Burn Maya jobs
 *
 */

class QTimer;

/// \ingroup ABurner
/// @{

class MayaBurner : public JobBurner
{
Q_OBJECT
public:
	MayaBurner( const JobAssignment & jobAssignment, Slave * slave );
	~MayaBurner();

	virtual QStringList processNames() const;

public slots:
	void startProcess();
	void cleanup();

public slots:
	void slotProcessOutputLine( const QString &, QProcess::ProcessChannel );

protected:
	QString buildCmdMaya7();
	QString buildCmdMaya() const;
	QString executable() const;
	QStringList environment();
	QString rendererFlag( const QString & ) const;

	QString mCmdString;
	int mFrame;
	QString mErrorOutput;
	QRegExp mCompleteRE, mAssignedRE, mFrameCompleteRE;
};

/// @}

#endif // MAYA_BURNER_H

#endif

