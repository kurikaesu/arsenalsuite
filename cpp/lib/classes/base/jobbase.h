
/*
 *
 * Copyright 2008 Blur Studio Inc.
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

#ifdef HEADER_FILES
#include "joboutput.h"
#include "mapping.h"

#include "trigger.h"
class JobTrigger : public Trigger
{
public:
	JobTrigger();
	virtual Record preUpdate( const Record & /*updated*/, const Record & /*before*/ );
};
#endif

#ifdef CLASS_FUNCTIONS
	static bool updateJobStatuses( JobList jobs, const QString & jobStatus, bool resetTasks, bool restartHosts = false );

	void changeFrameRange( QList<int>, JobOutput output = JobOutput(), bool changeCancelledToNew = true );

	// If hosts are added(or uncancelled) to a done job, the jobs status will be updated to suspended
	void changePreassignedTaskListWithStatusPrompt( HostList hosts, QWidget * parent = 0, bool changeCancelledToNew = false );
	
	// If updateStatusIfNeeded is true, and hosts are added(or uncancelled) to a done job, the jobs status will be updated to suspended
	// Returns the number of tasks that were added or uncancelled, used by changePreassignedTaskListWithStatusPrompt.
	int changePreassignedTaskList( HostList hosts, bool changeCancelledToNew = false, bool updateStatusIfNeeded = true );

	void addHistory( const QString & message );

	// Returns the proper mapping entries for this job.  The mapping entries are defined by the JobTypeMapping and JobMapping tables.
	// JobMappings override any JobTypeMappings that have the same mount
	MappingList mappings() const;
#endif

#ifdef TABLE_CTOR
	addTrigger( new JobTrigger() );
#endif
