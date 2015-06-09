
#include <qobject.h>
#include <qstring.h>

#include "server.h"
#include "xmpp.h"
#include "threadnotify.h"

using XMPP::Message;

class Session: public QObject
{
Q_OBJECT

public:
	Session();
	
	void connectToHost( const QString & /*host*/, int /*port*/, const QString & /*username*/,
						const QString & /*pass*/, const QString & /*resource*/);

public slots:

	void client_messageReceived( const Message & );
	
	void cs_connected();
	void cs_needAuthParams(bool, bool, bool);
	void cs_authenticated();
	void cs_connectionClosed();
	void cs_delayedCloseFinished();
	void cs_warning(int);
	void cs_error(int);

	void cs_incomingXml( const QString & );
	void cs_outgoingXml( const QString & );
	
	void slotThreadNotifysAdded( ThreadNotifyList );
	
private:
	enum{
		Offline,
		Connecting,
		Authing,
		Connected
	};
	/* Connect settings */
	QString mHost, mUser, mPass, mResource;
	int mPort;
	
	/* Class state */
	int mState;
	
	XMPP::Client * mClient;
	XMPP::ClientStream * mStream;
	XMPP::AdvancedConnector * mConnector;
	
	Server server;
};
