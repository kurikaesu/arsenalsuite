
#include <qfile.h>
#include <qtextstream.h>
#include <qprocess.h>

#include "server.h"
#include "group.h"
#include "usergroup.h"
#include "host.h"
#include "user.h"

Server::Server()
{
}

void Server::groupList(XMPP::RPC & c)
{
	GroupList gl = Group::select();
	foreach( GroupIter, it, gl )
		c.ResponseParams += (*it).name();
	c.ResponseParams += "Host";
	c.sendResponse();
}

void Server::group_retrieveUsers(XMPP::RPC &c)
{
	QString name = c.Params[0];
	if( name == "Host" ) {
		c.ResponseParams = Host::select().name();
		c.sendResponse();
	} else {
		Group g = Group::recordByName( name );
		if( g.isRecord() ) {
			c.ResponseParams = UserGroup::recordsByGroup( g ).user().name();
			c.sendResponse();
		} else
			c.sendError();
	}
}

void Server::group_emailUsers(XMPP::RPC & )
{
//	Worker * w = new Worker;
//	w->sendEmail(c);
}

void Server::group_addUser(XMPP::RPC & )
{
}

void Server::user_getHost(XMPP::RPC &c)
{
	QString name = c.Params[0];
	User u = User::recordByUserName( name );
	Host h;
	
	if( u.isRecord() )
		h = u.host();
	else
		h = Host::recordByName( name );
	
	if( !h.isRecord() ) {
		c.sendError();
		return;
	}
	
	c.ResponseParams += h.name();
	c.sendResponse();
}

