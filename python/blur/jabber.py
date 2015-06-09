
import sys,os,xmpp

class farmBot:
    def __init__(self, user, password, nick='Render Farm'):
        self.sender_jid = xmpp.protocol.JID(str(user))
        self.user = user
        self.nick = nick
        self.password = password

        self.connected = False

        self.rooms = []

        self.client = xmpp.Client(self.sender_jid.getDomain(),debug=[])
        self.disconnectHandler()

    def disconnectHandler(self):
        self.client.connect(secure=0,use_srv=False)
        if self.client.isConnected():
            self.client.auth(self.sender_jid.getNode(),str(self.password),self.sender_jid.getResource())
            self.client.sendPresence(typ='available')

            self.client.RegisterHandler('message', self.message)
            self.client.RegisterDisconnectHandler( self.disconnectHandler )

            if len(self.rooms) > 0:
                del self.rooms

            self.rooms = []

    def process(self):
        if not self.client.isConnected():
            self.disconnectHandler()
            if not self.client.isConnected():
                return

        self.client.Process()

    def joinRoom(self, room):
        if self.client.isConnected():
            self.client.sendPresence("%s/%s" % (room, self.nick))
            if not room in self.rooms:
                self.rooms.append(room)

    def leaveRoom(self, room):
        if self.client.isConnected():
            self.client.sendPresence("%s/%s" % (room, self.nick), typ='unavailable')
            if room in self.rooms:
                self.rooms.remove(room)

    def logOut(self):
        if self.client.isConnected():
            for room in self.rooms:
                self.client.sendPresence("%s/%s" % (room, self.nick), typ='unavailable')

            self.client.sendPresence(typ='unavailable')

    def send(self, receiver, text, sendType='chat'):
        if self.client.isConnected():
            receiver_jid = xmpp.protocol.JID(str(receiver))
            self.client.send(xmpp.protocol.Message(str(receiver),str(text),typ=sendType))

    # drop the message
    def message(self, conn, mess):
        pass

# Legacy compat below

# Logs into the sender jid's jabber server and authenticates
# with password.
# Sends a message to receiver with text as the contents
# The message is type='chat' unless sendAsChat=False
def send( sender, password, receiver, text, sendAsChat=True ):
	sender_jid = xmpp.protocol.JID(str(sender))
	receiver_jid = xmpp.protocol.JID(str(receiver))
	
	client = xmpp.Client(sender_jid.getDomain(),debug=[])
	client.connect(secure=0,use_srv=False)
	client.auth(sender_jid.getNode(),str(password),sender_jid.getResource())
	
	sendType = None # Regular Message
	if sendAsChat:
		sendType = 'chat'
	client.send(xmpp.protocol.Message(str(receiver),str(text),typ=sendType))
