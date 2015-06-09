
#include <qmessagebox.h>

#include "database.h"

#include "host.h"
#include "jobhistory.h"
#include "jobhistorytype.h"
#include "jobmapping.h"
#include "jobtask.h"
#include "jobdep.h"
#include "jobtable.h"
#include "jobtype.h"
#include "jobtypemapping.h"
#include "user.h"
#include "group.h"
#include "usergroup.h"

bool Job::updateJobStatuses( JobList jobs, const QString & jobStatus, bool resetTasks, bool restartHosts )
{
	if( jobs.isEmpty() )
		return false;

	QString keys = jobs.keyString();

	Database::current()->beginTransaction();

	if( restartHosts ){
		// Update each of the hosts that were working on the job
		if( !Database::current()->exec("UPDATE HostStatus SET slaveStatus = 'starting' WHERE fkeyJob IN(" + keys + ");").isActive() ) {
			Database::current()->rollbackTransaction();
			return false;
		}
	}

	if( resetTasks ) {
		foreach( Job j, jobs ) {
			JobDepList jdl = JobDep::recordsByJob(j);
			JobTaskList jtl = j.jobTasks().filter("status", "cancelled",  /*keepMatches*/ false);
			bool isSoftDep = false;
			foreach(JobDep jd, jdl){
				if( jd.depType() == 2 ) {
					isSoftDep = true;
					break;
				}
			}

			if( !isSoftDep )
				jtl.setStatuses("new");
			else
				jtl.setStatuses("holding");

			jtl.setColumnLiteral("fkeyjoboutput","NULL");
			if( j.packetType() != "preassigned" )
				jtl.setHosts(Host());
			jtl.commit();
		}
	}

	if( !jobStatus.isEmpty() ){

		foreach( Job j, jobs )
			if( j.status() != jobStatus )
				j.addHistory( "Status change from" + j.status() + " to " + jobStatus );

		// Update each of the Job records
		QString select("UPDATE Job SET status = '%1'");
		if( jobStatus == "new" )
			select += ", submitted=extract(epoch from now())";
		select = QString( select + " WHERE keyJob IN("+ keys + ");" ).arg(jobStatus);
		if( !Database::current()->exec( select ).isActive() ) {
			Database::current()->rollbackTransaction();
			return false;
		}
		
	}


	Database::current()->commitTransaction();
	return true;
}

void Job::changeFrameRange( QList<int> frames, JobOutput output, bool changeCancelledToNew )
{
	// Gather existing tasks for this output
	QString sql( "fkeyjob=?" );
	VarList args;
	args << key();
	if( output.isRecord() ) {
		sql += " AND fkeyjoboutput=?";
		args << output.key();
	}

	JobTaskList tasks = JobTask::select( sql, args );

	// Create tasks that are missing
	JobTaskList allTasks;
	QMap<int,JobTaskList> tasksByFrame = tasks.groupedBy<int,JobTaskList>( "frameNumber" );
	foreach( int frame, frames ) {
		if( tasksByFrame.contains( frame ) ) {
			foreach( JobTask task, tasksByFrame[frame] ) {
				if( changeCancelledToNew && task.status() == "cancelled" )
					task.setStatus( "new" );
				allTasks += task;
			}
		} else {
			JobTask jt;
			jt.setStatus( "new" );
			jt.setJob( *this );
			jt.setFrameNumber( frame );
			jt.setJobOutput( output );
			allTasks += jt;
		}
	}
	
	// Gather tasks that need to be canceled
	JobTaskList canceled;
	for( QMap<int,JobTaskList>::iterator it = tasksByFrame.begin(); it != tasksByFrame.end(); ++it ) {
		foreach( JobTask jt, it.value() )
			if( !allTasks.contains( jt ) )
				canceled += jt;
	}
	if( packetType() != "preassigned" )
		canceled.setHosts( Host() );
	canceled.setStatuses( "cancelled" );
	
	// Commit changes
	Database::current()->beginTransaction();
	allTasks.commit();
	canceled.commit();
	Database::current()->exec( "SELECT update_job_task_counts(?);", VarList() << key() );
	Database::current()->commitTransaction();

	addHistory( "Job Frame List Changed" );
}

void Job::changePreassignedTaskListWithStatusPrompt( HostList hosts, QWidget * parent, bool changeCancelledToNew )
{
	int newTasksCount = changePreassignedTaskList( hosts, changeCancelledToNew, /* updateStatusIfNeeded = */ false );
	if( status() == "done" && newTasksCount > 0 ) {
		QMessageBox * mb = new QMessageBox(parent);
		mb->setWindowTitle( "Job no longer done, choose next status" );
		mb->setText( "You have the option to set the job status to Ready or Suspended.\n" );
		mb->setDefaultButton( mb->addButton( "Suspended", QMessageBox::AcceptRole ) );
		mb->addButton( "Ready", QMessageBox::RejectRole );
		mb->exec();
		int role =  mb->buttonRole(mb->clickedButton());
		delete mb;
		updateJobStatuses( JobList() += *this, role == QMessageBox::AcceptRole ? "suspended" : "ready", false, false );
	}
}

int Job::changePreassignedTaskList( HostList hosts, bool changeCancelledToNew,  bool updateStatusIfNeeded )
{
	JobTaskList tasks = jobTasks(), toCancel, toCommit;
	HostList current = tasks.hosts();
	HostList hostsToAdd = hosts - current, hostsToCancel = current - hosts;
	HostList hostsToUncancel = current - hostsToCancel;

	int maxTaskNumber = 0;
	foreach( JobTask task, tasks ) {
		maxTaskNumber = qMax(task.frameNumber(),maxTaskNumber);
		if( hostsToCancel.contains( task.host() ) )
			toCancel += task;
		if( changeCancelledToNew && hostsToUncancel.contains( task.host() ) && task.status() == "cancelled" ) {
			task.setStatus( "new" );
			toCommit += task;
		}
	}

	toCancel.setStatuses( "cancelled" );

	foreach( Host host, hostsToAdd ) {
		JobTask jt;
		jt.setJob(*this);
		jt.setStatus( "new" );
		jt.setHost( host );
		jt.setFrameNumber( ++maxTaskNumber );
		toCommit += jt;
	}

	Database::current()->beginTransaction();
	toCommit.commit();
	toCancel.commit();
	Database::current()->exec( "SELECT update_job_task_counts(?);", VarList() << key() );
	Database::current()->commitTransaction();

	if( updateStatusIfNeeded && status() == "done" && hostsToAdd.size() + hostsToUncancel.size() > 0 )
		updateJobStatuses( JobList() += *this, "suspended", false, false );
	
	addHistory( "Job Frame List Changed" );
	return hostsToAdd.size() + hostsToUncancel.size();
}

MappingList Job::mappings() const
{
	MappingList mappings = jobType().jobTypeMappings().mappings();
	MappingList jobMappings = this->jobMappings().mappings();
	if( jobMappings.size() ) {
		QMap<QString,Mapping> byMount = mappings.groupedBySingle<QString,Mapping>("mount");
		foreach( Mapping m, jobMappings )
			byMount[m.mount()] = m;
		mappings.clear();
		for( QMap<QString,Mapping>::iterator it = byMount.begin(); it != byMount.end(); ++it )
			mappings.append( it.value() );
	}
	return mappings;
}

void Job::addHistory( const QString & message )
{
	if( message.isEmpty() )
		return;
	JobHistory jh;
	jh.setJob( *this );
	jh.setMessage( message );
	jh.setUser( User::currentUser() );
	jh.setHost( Host::currentHost() );
	jh.setColumnLiteral( "created", "NOW()" );
	jh.commit();
}

JobTrigger::JobTrigger()
: Trigger( Trigger::PreUpdateTrigger )
{}

Record JobTrigger::preUpdate( const Record & updated, const Record & /*before*/ )
{
	Job(updated).addHistory(updated.changeString());
	return updated;
}
