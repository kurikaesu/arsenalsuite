
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

#ifdef COMPILE_RIBGEN_BURNER

#ifndef RIBGEN_BURNER_H
#define RIBGEN_BURNER_H

#include "jobburner.h"

/*
 * This class is used to start,
 * and moniter RibGen using
 *
 */

class QTimer;

/// \ingroup ABurner
/// @{

class RibGenBurner : public JobBurner
{
Q_OBJECT
public:
	RibGenBurner( const JobAssignment & jobAssignment, Slave * slave );
	~RibGenBurner();

	virtual QStringList processNames() const;

public slots:
	void startProcess();

public slots:
	void slotProcessOutputLine( const QString &, QProcess::ProcessChannel );

protected:
	QString buildCmdRibGen();
	QString executable() const;
	QString rendererFlag( const QString & ) const;

	int mFrame;
	int mFrameEnd;
	QRegExp mFrameCompleteRE;
};

/// @}

#endif // RIBGEN_BURNER_H

#endif

