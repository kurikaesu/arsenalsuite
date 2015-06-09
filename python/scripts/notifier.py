#!/usr/bin/python

from PyQt4.QtCore import *
from PyQt4.QtSql import *
from blur.Stone import *
from blur.Classes import *
import blur.email, blur.jabber
import sys, time, re
from math import ceil
import traceback

try:
	import popen2
except: pass

if sys.argv.count('-daemonize'):
	from blur.daemonize import createDaemon
	createDaemon()

app = QCoreApplication(sys.argv)

initConfig( "/etc/notifier.ini", "/var/log/ab/notifier.log" )
# Read values from db.ini, but dont overwrite values from notifier.ini
# This allows db.ini defaults to work even if notifier.ini is non-existent
config().readFromFile( "/etc/db.ini", False )

blur.RedirectOutputToLog()

classes_loader()

VERBOSE_DEBUG = False

if VERBOSE_DEBUG:
	Database.current().setEchoMode( Database.EchoUpdate | Database.EchoDelete )# | Database.EchoSelect )

Database.current().connection().reconnect()

n = Notification.create( 'Notifier', 'Starting', 'Notifier is starting' )
#newellm = User.recordByUserName( 'newellm' )
#n.sendTo( newellm )#, 'Jabber' )
#n.sendTo( newellm, 'Email' )

def notify_jabber( dest ):
	notif = dest.notification()
	body = ''
	if not notif.subject().isEmpty():
		body = 'Subject: ' + notif.subject() + '\n'
	if not notif.message().isEmpty():
		body += notif.message()
	recip = dest.destination()
	blur.jabber.send('thepipe@jabber.blur.com/Reaper','thePIpe',str(recip), body )

def notify_email( dest ):
	recip = dest.destination()
	notif = dest.notification()
	blur.email.send( sender = 'thePipe@blur.com', recipients = [str(recip)], subject = notif.subject(), body = notif.message() )

def fill_default_destination( dest ):
	meth = dest.notificationMethod().name()
	recip = ''
	if meth == 'Email':
		recip = dest.user().email()
		if not '@' in recip:
			recip += '@blur.com'
	elif meth == 'Jabber':
		recip = dest.user().jid()
		if not '@' in recip:
			recip += '@jabber.blur.com'
	Log( "User's %s notification has no destination, setting to %s" % (meth,recip) )
	dest.setDestination( recip )
	return dest

def get_routes( notif, user ):
	sql = ('(eventMatch IS NULL or eventMatch ~ ?) AND (componentMatch IS NULL OR componentMatch ~ ?) AND (subjectMatch IS NULL OR subjectMatch ~ ?) ' +
			'AND (messageMatch IS NULL OR messageMatch ~ ?)')
	args = [QVariant(notif.event()), QVariant(notif.component()), QVariant(notif.subject()), QVariant(notif.message())]
	if user.isRecord():
		sql += ' AND (fkeyUser=?)'
		args.append( QVariant(user.key()) )
	else:
		sql += ' AND (fkeyUser IS NULL)'
	return NotificationRoute.select( sql, args ).sorted( 'priority' )

def isDestinationUniqueAndValid(dest):
	method = dest.notificationMethod()
	if method.isRecord() and not dest.destination().isEmpty():
		return NotificationDestination.select( 'fkeyNotification=? AND fkeyNotificationMethod=? AND destination=? AND routed IS NOT NULL',
				[QVariant(dest.notification().key()), QVariant(method.key()), QVariant(dest.destination()) ] ).isEmpty()
	return False

class RouteAction:
	def __init__(self, parts):
		self.Action = parts[0]
		self.Method = parts[1]
		try: # These parts are not required
			self.Dest = parts[2]
			self.User = parts[3]
		except: pass
		
	def applyToNotification(self,notif):
		if self.Action == 'add':
			method = NotificationMethod.recordByName( self.Method )
			user = User()
			if hasattr(self,'User'):
				user = User.recordByUserName( self.User )
			if method.isRecord() and (user.isRecord() or (hasattr(self,'Dest') and self.Dest)):
				nd = NotificationDestination()
				nd.setNotification( notif )
				nd.setUser( user )
				nd.setNotificationMethod( method )
				if hasattr( self, 'Dest' ):
					nd.setDestination( self.Dest )
				return nd
		return None
	
	def applyToDestination(self,dest,needsDefault):
		if self.Action == 'add':
			newDest = self.applyToNotification(dest.notification())
			# Route adds a new destination, set as owned by same user
			if not newDest.user().isRecord() and dest.user().isRecord():
				newDest.setUser( dest.user() )
			
			return (self.applyToNotification(dest.notification()),dest)
		elif needsDefault and self.Action == 'default':
			method = NotificationMethod.recordByName( self.Method )
			if method.isRecord():
				dest.setNotificationMethod( method )
				if hasattr( self,'Dest' ): # self.Dest may not exist
					dest.setDestination( self.Dest )
				return (None,dest)
		return (None,dest)
		
def parseActionString( actionString ):
	actions = []
	sections = actionString.split( ',' )
	for s in sections:
		parts = s.split(':')
		# Action:Method is required
		if len(parts) >= 2:
			actions.append( RouteAction( parts ) )
	return actions

def route_notification( notif ):
	Log( "Routing Notification: %i" % notif.key() )
	routes = get_routes( notif, User() )
	newDests = NotificationDestinationList()
	for route in routes:
		actions = parseActionString( route.actions() )
		for action in actions:
			res = action.applyToNotification( notif )
			if res:
				newDests += res
	newDests.commit()
	notif.setColumnLiteral( 'routed', 'NOW()' )
	notif.commit()
	
def route_destination( dest ):
	Log( "Routing Destination: %i" % dest.key() )
	routes = get_routes( dest.notification(), dest.user() )
	newDests = NotificationDestinationList()
	for route in routes:
		actions = parseActionString( route.actions() )
		for action in actions:
			res,dest = action.applyToDestination( notif, notif.destination().isEmpty() or not notif.notificationMethod().isRecord() )
			if res:
				newDests += res
	if dest.user().isRecord():
		if not dest.notificationMethod().isRecord():
			Log( "User Notification has no method, setting to Email" )
			dest.setNotificationMethod( NotificationMethod.recordByName( 'Email' ) )
		if dest.destination().isEmpty():
			dest = fill_default_destination( dest )
	if isDestinationUniqueAndValid( dest ):
		dest.setColumnLiteral( 'routed', 'NOW()' )
		dest.commit()
		# Only commit the new routes if the current is valid, to prevent infinit loop
		newDests.commit()
	else:
		Log( "Removing Destination because it is invalid or a duplicate: %i" % dest.key() )
		dest.remove()


# A dict, to map method names to their sending functions
methods = {
	'Jabber' : notify_jabber,
	'Email' : notify_email }

service = Service.ensureServiceExists('Notifier')

# Loop, Routing and Delivering the notifications
while True:
	# Route the notifications and destinations
	Log( "Routing Notifications" )
	service.pulse()
	for table, route_method in [ (Notification,route_notification), (NotificationDestination,route_destination) ]:
		to_route = table.select( "routed IS NULL" )
		for notif in to_route:
			route_method(notif)
	
	# Send to the routed destinations
	to_send = NotificationDestination.select( "delivered IS NULL AND routed IS NOT NULL" )
	
	# Sending Notifications
	for dest in to_send:
		try:
			methods[str(dest.notificationMethod().name())]( dest )
			dest.setColumnLiteral( 'delivered', 'NOW()' )
			dest.commit()
		except:
			Log( 'Couldnt find notification method for destination: %i' % dest.key() )
	
	time.sleep(8)
