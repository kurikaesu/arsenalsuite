
#ifndef SERVER_H
#define SERVER_H

#include <qobject.h>

#include "xmpp_tasks.h"

class Server : public QObject
{
Q_OBJECT
	
public:
	Server();

public slots:
	void groupList(XMPP::RPC & c);
	void group_retrieveUsers(XMPP::RPC &c);
	void group_emailUsers(XMPP::RPC &c);
	void group_addUser(XMPP::RPC &c);
	void user_getHost(XMPP::RPC &c);

};

#endif // SERVER_H

