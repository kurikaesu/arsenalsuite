
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

#ifdef COMPILE_BATCH_BURNER

#ifndef BATCH_BURNER_H
#define BATCH_BURNER_H

#include <qbytearray.h>

#include "jobburner.h"

class QProcess;
/// \ingroup ABurner
/// @{

class BatchBurner : public JobBurner
{
Q_OBJECT
public:
	BatchBurner( const JobAssignment &, Slave * slave );
	~BatchBurner();

	QString executable();
	QStringList environment();
	QString workingDirectory();

public slots:
	void startProcess();
	void slotProcessStarted();
	void slotProcessExited();
	
};

/// @}

#endif // BATCH_BURNER_H

#endif
