

#include <qstring.h>

#include <iostream>
#include <stdlib.h>

#include "session.h"
#include "xmpp.h"
#include "xmpp_tasks.h"

#include "user.h"
#include "thread.h"
#include "threadnotify.h"
#include "updatemanager.h"
#include "path.h"
#include "database.h"

using namespace std;

Session::Session()
{
	mState = Offline;
}

void Session::connectToHost( 
	const QString & host, 
	int port, 
	const QString & username, 
	const QString & pass, 
	const QString & resource)
{
	qWarning( "Session::connectToHost" );
	
	if( mState != Offline )
		return;
	
	mHost = host;
	mPort = port;
	mUser = username;
	mPass = pass;
	mResource = resource;
	
	mState = Connecting;

	mClient = new XMPP::Client();
	
	XMPP::Jid jid;
	jid.set( mHost, mUser, mResource );
	
	/*
	d->client->setOSName(getOSName());
	d->client->setTimeZone(getTZString(), getTZOffset());
	d->client->setClientName(PROG_NAME);
	d->client->setClientVersion(PROG_VERSION);

	d->client->setFileTransferEnabled(true);
*/

	connect(mClient, SIGNAL(messageReceived(const Message &)), SLOT(client_messageReceived(const Message &)));

	XMPP::AdvancedConnector::Proxy proxy;
	mConnector = new XMPP::AdvancedConnector();
	mConnector->setProxy( proxy );
	mConnector->setOptHostPort( mHost, mPort );
	mConnector->setOptSSL( false );
	
	mStream = new XMPP::ClientStream( mConnector );
	mStream->setOldOnly( true );
	mStream->setAllowPlain( true );
	mStream->setNoopTime(55000);
	mStream->setRequireMutualAuth( false );
	
	connect(mStream, SIGNAL(connected()), SLOT(cs_connected()));
	connect(mStream, SIGNAL(needAuthParams(bool, bool, bool)), SLOT(cs_needAuthParams(bool, bool, bool)));
	connect(mStream, SIGNAL(authenticated()), SLOT(cs_authenticated()));
	connect(mStream, SIGNAL(connectionClosed()), SLOT(cs_connectionClosed()));
	connect(mStream, SIGNAL(delayedCloseFinished()), SLOT(cs_delayedCloseFinished()));
	connect(mStream, SIGNAL(warning(int)), SLOT(cs_warning(int)));
	connect(mStream, SIGNAL(error(int)), SLOT(cs_error(int)));
	//connect(mStream, SIGNAL(incomingXml(const QString&)), SLOT(cs_incomingXml(const QString&)));
	//connect(mStream, SIGNAL(outgoingXml(const QString&)), SLOT(cs_outgoingXml(const QString&)));
	
	mClient->connectToServer( mStream, jid );
	
	UpdateManager::instance();
	connect( ThreadNotify::table(), SIGNAL( threadNotifysAdded( ThreadNotifyList ) ),
		SLOT( slotThreadNotifysAdded( ThreadNotifyList ) ) );
}

void Session::cs_incomingXml( const QString & xml )
{
	qWarning( "-------INCOMING XML-----------" );
	qWarning( xml );
}

void Session::cs_outgoingXml( const QString & xml )
{
	qWarning( "-------OUTGOING XML------------" );
	qWarning( xml );
}

void Session::cs_connected()
{
	qWarning( "Session::cs_connected" );
}

void Session::cs_needAuthParams(bool user, bool pass, bool realm)
{
	qWarning( "Session::cs_needAuthParams" );
	
	if(user) {
		qWarning( "Setting Username" );
		mStream->setUsername( mUser );
	}
	if(realm) {
		qWarning( "Setting Host" );
		mStream->setRealm( mHost );
	}
	if(pass) {
		qWarning( "Setting Password" );
		mStream->setPassword(mPass);
	}
	
	mStream->continueAfterParams();
}

void Session::cs_authenticated()
{
	qWarning( "Session::cs_authenticated" );
	mClient->start( mHost, mUser, mPass, mResource );
	mClient->setPresence( XMPP::Status( "Online", "Online", 5 ) );
	
	mState = Connected;
	XMPP::JT_RPCListener * rpc = new XMPP::JT_RPCListener(mClient->rootTask());
	rpc->addProcedure("groupList", &server, SLOT( groupList(RPC &) ) );
	rpc->addProcedure("group_retrieveUsers", &server, SLOT( group_retrieveUsers(RPC &) ) );
	rpc->addProcedure("group_emailUsers", &server, SLOT( group_emailUsers(RPC &) ) );
	rpc->addProcedure("group_addUser", &server, SLOT( group_addUser(RPC &) ) );
	rpc->addProcedure("user_getHost", &server, SLOT( user_getHost(RPC &) ) );
}

void Session::cs_connectionClosed()
{
	qWarning( "Session::cs_connectionClosed" );
	delete mClient;
	mState = Offline;
	return;
}

void Session::cs_delayedCloseFinished()
{
	qWarning( "Session::cs_delayedCloseFinished" );
}

void Session::cs_warning(int warn)
{
	qWarning( "Session::cs_warning" );
	
	if(warn == XMPP::ClientStream::WarnOldVersion) {
		qWarning("Warning: pre-1.0 protocol server");
	}
	else if(warn == XMPP::ClientStream::WarnNoTLS) {
		qWarning("Warning: TLS not available!" );
	}
	
	mStream->continueAfterWarning();
}

void Session::cs_error(int err)
{
	qWarning( "Session::cs_error" );
	
	if(err == XMPP::ClientStream::ErrParse) {
		qWarning("XML parsing error");
	}
	else if(err == XMPP::ClientStream::ErrProtocol) {
		qWarning("XMPP protocol error");
	}
	else if(err == XMPP::ClientStream::ErrStream) {
		int x = mStream->errorCondition();
		QString s;
		if(x == XMPP::Stream::GenericStreamError)
			s = "generic stream error";
		else if(x == XMPP::ClientStream::Conflict)
			s = "conflict (remote login replacing this one)";
		else if(x == XMPP::ClientStream::ConnectionTimeout)
			s = "timed out from inactivity";
		else if(x == XMPP::ClientStream::InternalServerError)
			s = "internal server error";
		else if(x == XMPP::ClientStream::InvalidFrom)
			s = "invalid from address";
		else if(x == XMPP::ClientStream::InvalidXml)
			s = "invalid XML";
		else if(x == XMPP::ClientStream::PolicyViolation)
			s = "policy violation.  go to jail!";
		else if(x == XMPP::ClientStream::ResourceConstraint)
			s = "server out of resources";
		else if(x == XMPP::ClientStream::SystemShutdown)
			s = "system is shutting down NOW";
		qWarning(QString("XMPP stream error: %1").arg(s));
	}
	else if(err == XMPP::ClientStream::ErrConnection) {
		int x = mConnector->errorCode();
		QString s;
		if(x == XMPP::AdvancedConnector::ErrConnectionRefused)
			s = "unable to connect to server";
		else if(x == XMPP::AdvancedConnector::ErrHostNotFound)
			s = "host not found";
		else if(x == XMPP::AdvancedConnector::ErrProxyConnect)
			s = "proxy connect";
		else if(x == XMPP::AdvancedConnector::ErrProxyNeg)
			s = "proxy negotiating";
		else if(x == XMPP::AdvancedConnector::ErrProxyAuth)
			s = "proxy authorization";
		else if(x == XMPP::AdvancedConnector::ErrStream)
			s = "stream error";
		qWarning(QString("Connection error: %1").arg(s));
	}
	else if(err == XMPP::ClientStream::ErrNeg) {
		int x = mStream->errorCondition();
		QString s;
		if(x == XMPP::ClientStream::HostGone)
			s = "host no longer hosted";
		else if(x == XMPP::ClientStream::HostUnknown)
			s = "host unknown";
		else if(x == XMPP::ClientStream::RemoteConnectionFailed)
			s = "a required remote connection failed";
		else if(x == XMPP::ClientStream::SeeOtherHost)
			s = QString("see other host: [%1]").arg(mStream->errorText());
		else if(x == XMPP::ClientStream::UnsupportedVersion)
			s = "server does not support proper xmpp version";
		qWarning(QString("Stream negotiation error: %1").arg(s));
	}
	else if(err == XMPP::ClientStream::ErrTLS) {
		int x = mStream->errorCondition();
		QString s;
		if(x == XMPP::ClientStream::TLSStart)
			s = "server rejected STARTTLS";
		else if(x == XMPP::ClientStream::TLSFail) {
//			int t = 0;//tlsHandler->tlsError();
	//		if(t == QCA::TLS::ErrHandshake)
		//		s = "TLS handshake error";
			//else
				//s = "broken security layer (TLS)";
		}
		qWarning(s);
	}
	else if(err == XMPP::ClientStream::ErrAuth) {
		int x = mStream->errorCondition();
		QString s;
		if(x == XMPP::ClientStream::GenericAuthError)
			s = "unable to login";
		else if(x == XMPP::ClientStream::NoMech)
			s = "no appropriate auth mechanism available for given security settings";
		else if(x == XMPP::ClientStream::BadProto)
			s = "bad server response";
		else if(x == XMPP::ClientStream::BadServ)
			s = "server failed mutual authentication";
		else if(x == XMPP::ClientStream::EncryptionRequired)
			s = "encryption required for chosen SASL mechanism";
		else if(x == XMPP::ClientStream::InvalidAuthzid)
			s = "invalid authzid";
		else if(x == XMPP::ClientStream::InvalidMech)
			s = "invalid SASL mechanism";
		else if(x == XMPP::ClientStream::InvalidRealm)
			s = "invalid realm";
		else if(x == XMPP::ClientStream::MechTooWeak)
			s = "SASL mechanism too weak for authzid";
		else if(x == XMPP::ClientStream::NotAuthorized)
			s = "not authorized";
		else if(x == XMPP::ClientStream::TemporaryAuthFailure)
			s = "temporary auth failure";
		qWarning(QString("Auth error: %1").arg(s));
	}
	else if(err == XMPP::ClientStream::ErrSecurityLayer)
		qWarning("Broken security layer (SASL)");
//	cleanup();
}

void Session::client_messageReceived( const XMPP::Message & m )
{
	qWarning( "Session::client_messageRecieved" );
	
	QString thread = m.thread();
	bool ok;
	uint threadKey = thread.toUInt( &ok );
	Thread t;
	if( ok )
		t = Thread( threadKey );
		
	if( !t.isRecord() ) {
		qWarning( "Incoming message with unknown thread: " + thread );
		return;
	}
	
	User sender( User::recordByUserName( m.from().user() ) );
	if( !sender.isRecord() ) {
		qWarning( "Could not get a user for: " + m.from().user() );
		return;
	}
	
	qWarning( "Posting reply" );
	
	Database::instance()->beginTransaction();
	
	Thread reply;
	reply.setElement( t.element() );
	reply.setReply( t );
	reply.setTopic( m.subject() + "[JABBER]" );
	reply.setBody( m.body() );
	reply.setUser( sender );
	reply.setDateTime( m.timeStamp() );
	reply.commit();
	
	ThreadNotifyList to_commit, tul = ThreadNotify::recordsByThread( t );
	UserList to_notify;
	
	// Gather all users except the sender of the reply
	foreach( ThreadNotifyIter, it, tul )
		if( (*it).user() != sender )
			to_notify += (*it).user();
	
	// Add the sender of the message we are replying to
	if( !to_notify.contains( t.user() ) )
		to_notify += t.user();
	
	foreach( UserIter, it, to_notify ) {
		ThreadNotify tu;
		tu.setUser( *it );
		tu.setThread( reply );
		to_commit += tu;
	}
	
	to_commit.commit();
	
	Database::instance()->commitTransaction();
}


void Session::slotThreadNotifysAdded( ThreadNotifyList tnl )
{
	qWarning( "Session::slotThreadNotifysAdded" );
	foreach( ThreadNotifyIter, it, tnl )
	{
		User u( (*it).user() );
		if( !u.threadNotifyByJabber() )
			continue;
			
		Thread t( (*it).thread() );
		
		XMPP::Jid to;
		to.set( "jabber.blur.com", u.name(), "" );
		
		ThreadNotifyList rl = ThreadNotify::recordsByThread( t );
		QStringList rec;
		foreach( ThreadNotifyIter, it, rl ) {
			if( (*it).user().isRecord() )
				rec += (*it).user().displayName();
		}
		
		XMPP::Message m;
		m.setSubject( t.topic() );
		m.setThread( QString::number( t.key() ) );
		QString body( "--------------------------------------------------\n"
			"This is a message from Resin resin://element=%1\n"
			"From %2 to %3 regarding %4\n"
			"--------------------------------------------------\n" );
		body = body.arg( t.element().key() ).arg( t.user().displayName() ).arg( rec.join(",") ).arg( t.element().displayPath() ) + t.body();
		
		QStringList afl = t.attachmentFiles();
		if( !afl.isEmpty() ) {
			body += "\n-----------------------------------------------\n";
			body += " Attachments\n";
			QString path( Path::winPath( t.attachmentsPath() ) );
			foreach( QStringList::Iterator, it, afl )
				body += "file://" + path + *it + "\n";
		}
		m.setBody( body );
		m.setTo( to );
		m.setFrom( mClient->jid() );
		mClient->sendMessage( m );
	}
}


