
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


#ifndef JOB_BURNER_FACTORY_H
#define JOB_BURNER_FACTORY_H

#include <qstring.h>
#include <qmap.h>

class Job;
class JobAssignment;
class JobBurner;
class JobBurnerPlugin;
class Slave;

class JobBurnerFactory
{
	public:
	static bool supportsJobType( const Job & job );
	static JobBurner * createBurner( const JobAssignment & jobAssignment, Slave * slave, QString * errMsg );
	static void registerPlugin( JobBurnerPlugin * bp, bool overrideExisting = false );
	static QMap<QString,JobBurnerPlugin*>  mBurnerPlugins;
	static bool mPluginsLoaded;
};

#endif // JOB_BURNER_FACTORY_H
