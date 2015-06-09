
#include <stdlib.h>

#include <qdir.h>
#include <qstring.h>

#include "spewer.h"

#include "updatemanager.h"
#include "freezercore.h"

#include "threadnotify.h"
#include "user.h"

static void notifyByEmail( Thread t, QStringList users )
{
	if( users.isEmpty() )
		return;
		
 	QString body( t.body() );
	body += "\n\n--brought to you by Resin\nresin://Element=" + QString::number(t.element().key()) + "&View=Notes";
	
	QString cmd( "perl -MBlur::Service::EMail -e'Blur::Service::EMail->new()->send( recipients => [\"%1\"], subject => \"%2\","
	" user => \"%3\", body => \"%4\", replyto=> \"resin-note-%5\\@blur.com\", attachments => [%6] )'" );
	cmd = cmd.arg( users.join("\",\"") ).arg( t.topic() ).arg( t.user().name() );
	cmd = cmd.arg( body ).arg( t.key() );
	QString attachpath = "/mnt/animation/Attachments/" + QString::number( t.key() ) + "/";
	QStringList attachments = QDir( attachpath ).entryList( QDir::Files );
	foreach( QStringList::Iterator, it, attachments )
		(*it) = '"' + attachpath + *it + '"';
	cmd = cmd.arg( attachments.join(",") );
	qWarning( cmd );
	system( cmd.utf8() );
}

/*
// This is now done in the resin jabber daemon
static void notifyByJabber( Thread t, QStringList users )
{
	if( users.isEmpty() )
		return;
		
 	QString body( t.body() );
	body += "\n\n--brought to you by Resin\nresin://Element=" + QString::number(t.element().key()) + "&View=Notes";

	QString cmd( "perl -MBlur::Service::Jabber -e'Blur::Service::Jabber->new()->send( recipients => [\"%1\"],"
	" subject => \"%2\", body => \"%3\" )'" );
	cmd = cmd.arg( users.join("\",\"") ).arg( t.topic() ).arg( body );
	qWarning( cmd );
	system( cmd.utf8() );
}
*/

Spewer::Spewer( QObject * parent )
: QObject( parent )
{
	FreezerCore::instance();
	UpdateManager::instance();
	connect( Thread::table(), SIGNAL( threadsAdded( ThreadList ) ), SLOT( threadsAdded( ThreadList ) ) );
}
	
void Spewer::threadsAdded( ThreadList tl )
{
	foreach( ThreadIter, it, tl )
	{
		QStringList jabber, email;
		ThreadNotifyList tnl = ThreadNotify::recordsByThread( *it );
		foreach( ThreadNotifyIter, tn_it, tnl ) {
			User u( (*tn_it).user() );
//			if( u.threadNotifyByJabber() )
//				jabber += u.name();
			if( u.threadNotifyByEmail() )
				email += u.name();
		}
		notifyByEmail( *it, email );
//		notifyByJabber( *it, jabber );
	}
}


