
#include "jobservice.h"
#include "job.h"
#include "service.h"

void JobServiceSchema::postInsert( RecordList jsl )
{
	foreach( JobService js, jsl )
		js.job().addHistory( "Service Added: " + js.service().service() );
}

void JobServiceSchema::postUpdate( const Record & updated, const Record & old )
{
	JobService js(updated), jso(old);
	js.job().addHistory( "Service changed from " + jso.service().service() + " to " + js.service().service() );
}

void JobServiceSchema::postDelete( RecordList jsl )
{
	foreach( JobService js, jsl )
		js.job().addHistory( "Service Removed: " + js.service().service() );
}

