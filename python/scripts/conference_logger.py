#!/usr/bin/python
# -*- coding: koi8-r -*-
import blur.Stone
import PyQt4.QtCore
from xmpp import *
import time,os,sys

app = PyQt4.QtCore.QCoreApplication(sys.argv)
blur.Stone.initConfig( 'conference_logger.ini', 'it@conference.blur.com.log' )
blur.RedirectOutputToLog()

#BOT=(botjid,password)
BOT=('thepipe@jabber.blur.com','thePIpe')
#CONF=(confjid,password)
CONF=('it@conference.jabber.blur.com','')

def LOG(stanza,nick,text):
	print nick, text

def messageCB(sess,mess):
	fro = mess.getFrom()
	print fro
	nick=fro.getResource()
	text=mess.getBody()
	LOG(mess,nick,text)

roster=[]
def presenceCB(sess,pres):
	nick=pres.getFrom().getResource()
	text=''
	if pres.getType()=='unavailable':
		if nick in roster:
			text=nick+' offline'
			roster.remove(nick)
	else:
		if nick not in roster:
			text=nick+' online'
			roster.append(nick)
	if text: LOG(pres,nick,text)

if 1:
	cl=Client(JID(BOT[0]).getDomain(),debug=[])
	cl.connect()
	cl.RegisterHandler('message',messageCB)
	cl.RegisterHandler('presence',presenceCB)
	cl.auth(JID(BOT[0]).getNode(),BOT[1])
	p=Presence(to='%s/logger'%CONF[0])
	p.setTag('x',namespace=NS_MUC).setTagData('password',CONF[1])
	p.getTag('x').addChild('history',{'maxchars':'0','maxstanzas':'0'})
	cl.send(p)
	while 1:
		cl.Process(1)
