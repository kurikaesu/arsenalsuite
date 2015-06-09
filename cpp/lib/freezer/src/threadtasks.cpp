
/* $Author: newellm $
 * $LastChangedDate: 2012-11-13 14:26:11 -0800 (Tue, 13 Nov 2012) $
 * $Rev: 13846 $
 * $HeadURL: svn://newellm@svn.blur.com/blur/trunk/cpp/lib/assfreezer/src/threadtasks.cpp $
 */

#include <qsqlquery.h>
#include <qsqlerror.h>

#include "database.h"

#include "employee.h"
#include "group.h"
#include "hostservice.h"
#include "permission.h"
#include "usergroup.h"
#include "jobstatus.h"

#include "threadtasks.h"

#define CHECK_CANCEL if( mCancel ) return;

static bool staticJobListDataRetrieved = false;

StaticJobListDataTask::StaticJobListDataTask( QObject * rec )
: ThreadTask( STATIC_JOB_LIST_DATA, rec )
, mHasData( false )
{}

void StaticJobListDataTask::run()
{
	if( !staticJobListDataRetrieved ) {
		mJobTypes = JobType::select();
		CHECK_CANCEL
		mProjects = Project::select( "WHERE keyelement in (SELECT fkeyproject FROM Job group by fkeyproject)" );
		CHECK_CANCEL
		// Ensure host table is preloaded
		Host::select();
		CHECK_CANCEL
		mServices = Service::select( "WHERE keyService IN (SELECT fkeyService FROM JobService)" );
		mHasData = staticJobListDataRetrieved = true;
		// Cache information needed for permission checks
		// Permission checks will happen when building menus
		Employee::select();
		Group::select();
		Permission::select();
		UserGroup::recordsByUser( User::currentUser() );
	}
}

StaticHostListDataTask::StaticHostListDataTask( QObject * rec )
: ThreadTask( STATIC_HOST_LIST_DATA, rec )
{}

void StaticHostListDataTask::run()
{
	mServices = Service::select();
}

JobListTask::JobListTask( QObject * rec, const JobFilter & jf, const JobList & jobList, ProjectList projects, bool fetchJobServices, bool needDeps )
: ThreadTask( JOB_LIST, rec )
, mJobFilter( jf )
, mJobList( jobList )
, mProjects( projects )
, mFetchJobServices( fetchJobServices )
, mFetchJobDeps( needDeps )
, mFetchUserServices( true )
, mFetchProjectSlots( true )
{}

void JobListTask::run()
{
//	User::select(); // Make sure usr is loaded
//	CHECK_CANCEL

	bool projectFilterShowingNone = (!mJobFilter.allProjectsShown && mJobFilter.visibleProjects.isEmpty() && !mJobFilter.showNonProjectJobs);

	if( mJobList.size() ) {
		mReturn = mJobList.reloaded();
	} else
	// If we have a project list, and all projects are hidden, then there will be no items, so just clear the list
	if( !(projectFilterShowingNone || mJobFilter.typesToShow.isEmpty() || mJobFilter.statusToShow.isEmpty() || mJobFilter.servicesToShow.isEmpty()) )
	{
		Expression e;
		
		//LOG_5( "Statuses to show: "   + mJobFilter.statusToShow.join(",") );
		//LOG_5( "Hidden Projects: " + mJobFilter.hiddenProjects.join(",") );
		//LOG_5( "Non-Project Jobs: " + QString(mJobFilter.showNonProjectJobs ? "true" : "false") );
		//LOG_5( "User List: " + mJobFilter.userList.join(",") );

			if( mJobFilter.statusToShow.size() > 0 && mJobFilter.statusToShow.size() != 9 )
			e = Job::c.Status.in(mJobFilter.statusToShow);

		if( mJobFilter.userList.size() > 0 )
			e &= Job::c.User.in(mJobFilter.userList);
		
		if( mJobFilter.servicesToShow.size() < mJobServices.size() )
			e &= Job::c.Key.in( Query( JobService::c.Job, JobService::c.Service.in(mJobFilter.servicesToShow) ) );

		if( mJobFilter.allProjectsShown && !mJobFilter.showNonProjectJobs )
			e &= Job::c.Project.isNotNull() & (Job::c.Project != ev_(0));
		else if( !mJobFilter.allProjectsShown ) {
			QList<uint> projectKeys = mJobFilter.visibleProjects;
			Expression sub;
			if( mJobFilter.showNonProjectJobs ) {
				projectKeys.append( 0 );
				sub = Job::c.Project.isNull();
			}
			if( projectKeys.size() )
				e &= (Job::c.Project.in(projectKeys) || sub);
		}
		if( mJobFilter.elementList.size() > 0 )
			e &= Job::c.Element.in(mJobFilter.elementList);

		if( mJobFilter.mDaysLimit > 0 )
			//e &= Job::c.Submittedts > Expression::now() - Interval(0,mJobFilter.mDaysLimit,0);
			e &= Expression::sql(QString(" AND submittedts > now()-'%1 days'::interval").arg(QString::number(mJobFilter.mDaysLimit)));

		if( mJobFilter.mExtraFilters.isValid() )
			e &= mJobFilter.mExtraFilters;

		e = e.orderBy( Job::c.Key, Expression::Descending ).limit(mJobFilter.mLimit);

		bool supportsMultiSelect = Database::current()->connection()->capabilities() & Connection::Cap_MultiTableSelect;
		TableList tables;

		CHECK_CANCEL
		foreach( uint type, mJobFilter.typesToShow ) {
			JobType jt( type );
			if( !jt.isRecord() ) {
				LOG_5( "JobType filter was not a valid record: " + jt.name() );
				continue;
			}
			Table * tbl = Database::current()->tableByName( "job" + jt.name() );
			if( !tbl ) {
				LOG_5( "Couldn't find table for jobtype: " + jt.name() );
				continue;
			}
			if( supportsMultiSelect )
				tables += tbl;
			else
				mReturn += tbl->selectOnly( e );
			CHECK_CANCEL
		}
		if( supportsMultiSelect )
			mReturn = Database::current()->tableByName( "job" )->selectMulti( tables, e );
		CHECK_CANCEL
	}

	JobList jobsNeedingRefresh;
	if( mFetchJobDeps && mReturn.size() ) {
		JobList jobsNeedingDeps = mReturn;
		Index * idx = JobDep::table()->indexFromField( "fkeyJob" );
		idx->cacheIncoming(true);
		mJobDeps = JobDep::table()->selectFrom( "jobdep_recursive('" + jobsNeedingDeps.keyString() + "') AS JobDep" );
		idx->cacheIncoming(false);
		QMap<uint,JobDepList> depsByJob = mJobDeps.groupedBy<uint,JobDepList>("fkeyJob");
		foreach( Job j, jobsNeedingDeps )
			if( !depsByJob.contains(j.key()) )
				idx->setEmptyEntry( QList<QVariant>() << j.getValue( "keyjob" ) );
		mDependentJobs = mJobDeps.deps();
		jobsNeedingRefresh = (mDependentJobs - mReturn);
		jobsNeedingRefresh.reload();
		CHECK_CANCEL
	}

	JobList allJobs = mReturn + jobsNeedingRefresh;
	if( allJobs.size() ) {
		allJobs.jobStatuses(Index::UseSelect); // Always does a full select
		CHECK_CANCEL

		if( mFetchJobServices )
			mJobServices = allJobs.jobServices(Index::UseSelect);
	}
	if( mFetchUserServices ) {
		QSqlQuery q = Database::current()->exec( "SELECT * FROM user_service_current" );
		while( q.next() ) {
			QString key = q.value( 0 ).toString();
			int value = q.value( 1 ).toInt();
			mUserServiceCurrent[key] = value;
		}
		QSqlQuery q2 = Database::current()->exec( "SELECT * FROM user_service_limits" );
		while( q2.next() ) {
			QString key = q2.value( 0 ).toString();
			int value = q2.value( 1 ).toInt();
			mUserServiceLimits[key] = value;
		}
	}
	CHECK_CANCEL
	if( mFetchProjectSlots ) {
		QSqlQuery q = Database::current()->exec( "SELECT * FROM project_slots_current" );
		while( q.next() ) {
			QString key = q.value( 0 ).toString();
			int projectSlots = q.value( 1 ).toInt();
			int reserve = q.value( 2 ).toInt();
			int limit = q.value( 3 ).toInt();
			mProjectSlots[key] = QString("%1:%2:%3").arg(projectSlots).arg(reserve).arg(limit);
		}
	}
}

HostListTask::HostListTask( QObject * rec, ServiceList serviceFilter, ServiceList activeServices, const Expression & extraFilters, bool loadHostServices, bool loadHostInterfaces )
: ThreadTask( HOST_LIST, rec )
, mServiceFilter( serviceFilter )
, mActiveServices( activeServices )
, mExtraFilters( extraFilters )
, mLoadHostServices( loadHostServices )
, mLoadHostInterfaces( loadHostInterfaces )
{
}

void HostListTask::run()
{
	if( !mServiceFilter.isEmpty() ) {
		Expression e;
		if( mServiceFilter.size() < mActiveServices.size() )
			e = Host::c.Key.in( Query( HostService::c.Host, HostService::c.Service.in(mServiceFilter) ) );
		e &= Host::c.Online == ev_(1);
		if( mExtraFilters.isValid() )
			e &= mExtraFilters;
		mReturn = e.select();
		if( mReturn.size() ) {
			CHECK_CANCEL
			mHostStatuses = HostStatus::select(HostStatus::c.Host.in(mReturn));
	//		CHECK_CANCEL
	//		mHostJobs = mHostStatuses.jobs();
		}
	}
	if( mLoadHostServices ) {
		CHECK_CANCEL
		// mReturn.hostServices();
		HostService::select();
	}
	if( mLoadHostInterfaces ) {
		CHECK_CANCEL
		mHostInterfaces = mReturn.hostInterfaces();
	}
}

FrameListTask::FrameListTask( QObject * rec, const Job & job )
: ThreadTask( FRAME_LIST, rec )
, mJob( job )
{}

void FrameListTask::run()
{
	mReturn = mJob.jobTasks(Index::UseSelect);
	CHECK_CANCEL
	mTaskAssignments = mReturn.jobTaskAssignments(Index::UseSelect);
	CHECK_CANCEL
	mOutputs = mJob.jobOutputs(Index::UseSelect);
	CHECK_CANCEL
	QSqlQuery q = Database::current()->exec( "SELECT now();" );
	if( q.next() )
		mCurTime = q.value( 0 ).toDateTime();
}

PartialFrameListTask::PartialFrameListTask( QObject * rec, const JobTaskList & jtl )
: ThreadTask( PARTIAL_FRAME_LIST, rec )
, mJtl( jtl )
{}

void PartialFrameListTask::run()
{
	if( mJtl.size() == 0 )
		return;
	mReturn = mJtl.reloaded();
	CHECK_CANCEL
	QSqlQuery q = Database::current()->exec( "SELECT now();" );
	if( q.next() )
		mCurTime = q.value( 0 ).toDateTime();
}

ErrorListTask::ErrorListTask( QObject * rec, const Job & job, bool fetchClearedErrors )
: ThreadTask( ERROR_LIST, rec )
, mFetchJobs(false)
, mFetchServices(false)
, mFetchClearedErrors( fetchClearedErrors )
, mLimit( 0 )
, mJobFilter(job)
{}

ErrorListTask::ErrorListTask( QObject * rec, const Host & host, int limit, bool showCleared )
: ThreadTask( ERROR_LIST, rec )
, mFetchJobs(true)
, mFetchServices(true)
, mFetchClearedErrors( showCleared )
, mLimit( limit )
, mHostFilter(host)
{}

ErrorListTask::ErrorListTask( QObject * rec, JobList jobFilter, HostList hostFilter, ServiceList serviceFilter, JobTypeList jobTypeFilter, const QString & messageFilter, bool showServices, int limit, const Expression & extraFilters )
: ThreadTask( ERROR_LIST, rec )
, mFetchJobs(true)
, mFetchServices(showServices)
, mFetchClearedErrors(true)
, mLimit( limit )
, mHostFilter(hostFilter)
, mJobFilter(jobFilter)
, mServiceFilter(serviceFilter)
, mJobTypeFilter(jobTypeFilter)
, mMessageFilter(messageFilter)
, mExtraFilters(extraFilters)
{}

void ErrorListTask::run()
{
	Expression e;
	if( mJobFilter.size() )
		e &= JobError::c.Job.in(mJobFilter); // fkeyJob IN (*mJobFilter*)
	if( mHostFilter.size() )
		e &= JobError::c.Host.in(mHostFilter); // fkeyHost IN (*mHostFilter*)
	if( !mFetchClearedErrors )
		e &= (JobError::c.Cleared == ev_(false)) || JobError::c.Cleared.isNull(); // (cleared = false OR cleared IS NULL)
	if( mServiceFilter.size() )
		e &= JobError::c.Job.in( Query( JobService::c.Job, JobService::c.Service.in(mServiceFilter) ) ); // fkeyJob IN (SELECT fkeyjob FROM JobService WHERE fkeyService IN (*mServiceFilter*))
	if( mJobTypeFilter.size() )
		e &= JobError::c.Job.in( Query( Job::c.Key, Job::c.JobType.in(mJobTypeFilter) ) ); // fkeyJob IN (SELECT keyJob FROM Job WHERE fkeyJobType IN (*jobTypeFilter*))
	if( mExtraFilters.isValid() )
		e &= mExtraFilters;
	if( mLimit ) {
		// If there's a limit fetch the most recent errors
		e = e.orderBy(JobError::c.LastOccurrence);
		e = e.limit(mLimit);
	}
	mReturn = JobError::select(e);
	if( mFetchJobs )
		mJobs = mReturn.jobs(Index::UseSelect);
	if( mFetchServices )
		mJobServices = JobService::c.Job.in(mReturn).select();
}

CounterTask::CounterTask( QObject * rec )
: ThreadTask( COUNTER, rec )
{}

void CounterTask::run()
{
	QSqlQuery query = Database::current()->exec("SELECT hostsTotal, hostsActive, hostsReady, jobsTotal, jobsActive, jobsDone FROM getcounterstate();");
	if( query.next() ) {
		mReturn.hostsTotal = query.value(0).toInt();
		mReturn.hostsActive = query.value(1).toInt();
		mReturn.hostsReady = query.value(2).toInt();
		mReturn.jobsTotal = query.value(3).toInt();
		mReturn.jobsActive = query.value(4).toInt();
		mReturn.jobsDone = query.value(5).toInt();
	}
	CHECK_CANCEL
	mManagerService = Service::recordByName( "AB_manager" );
}


JobHistoryListTask::JobHistoryListTask( QObject * rec, JobList jobs )
: ThreadTask( JOB_HISTORY_LIST, rec )
, mJobs( jobs )
{
}

void JobHistoryListTask::run()
{
	mReturn = mJobs.jobHistories();
}


UpdateJobListTask::UpdateJobListTask( QObject * rec, const JobList & jobs, const QString & status )
: ThreadTask( UPDATE_JOB_LIST, rec )
, mReturn( jobs )
, mStatus( status )
{
}

void UpdateJobListTask::run()
{
	Job::updateJobStatuses( mReturn, mStatus, false );
	//mReturn.setStatuses(mStatus);
	//mReturn.commit();
}

UpdateHostListTask::UpdateHostListTask( QObject * rec, const HostList & hosts, const QString & status )
: ThreadTask( UPDATE_JOB_LIST, rec )
, mReturn( hosts )
, mStatus( status )
{
}

void UpdateHostListTask::run()
{
	Database::current()->beginTransaction();

	HostStatusList hsl = mReturn.hostStatuses();
	hsl.setSlaveStatuses(mStatus);
	hsl.commit();

	QStringList returnTasksSql;
	foreach( Host h, mReturn )
		returnTasksSql += "return_slave_tasks_3(" + QString::number( h.key() ) + ")";

	Database::current()->exec("SELECT " + returnTasksSql.join(",") + ";");
	Database::current()->commitTransaction();
}

