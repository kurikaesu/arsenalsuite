#!/usr/bin/python

from PyQt4.QtCore import *
from PyQt4.QtSql import *
from blur.Stone import *
from blur.Classes import *
import blur
import sys
import time
import socket
import struct

if sys.argv.count('-daemonize'):
	from blur.daemonize import createDaemon
	createDaemon()

app = QCoreApplication(sys.argv)

initConfig( '/etc/energy_saver.ini', '/var/log/ab/energy_saver.log' )
initStone(sys.argv)
# Read values from db.ini, but dont overwrite values from energy_saver.ini
# This allows db.ini defaults to work even if energy_saver.ini is non-existent
config().readFromFile( "/etc/db.ini", False )

classes_loader()

blur.RedirectOutputToLog()

#Database.instance().setEchoMode( Database.EchoUpdate | Database.EchoDelete | Database.EchoSelect )

Database.current().connection().reconnect()

class ESConfig:
	def __init__(self):
		pass
	
	def update(self):
		self.percent_buffer = Config.getFloat("energySaverPercentBuffer",.20) # 20% default
		# Time in minutes for load average to go from one extreme to another
		self.load_avg_reset_time = Config.getInt("energySaverLoadAvgResetTime",15)

		# Seconds a host sits at 'ready' until it is considered idle
		self.time_before_idle = Config.getInt("energySaverTimeBeforeHostIdle",300)

		# Sleep hosts delay - Time in seconds between each host put to sleep
		self.sleep_host_delay = Config.getInt("energySaverHostSleepInterval",120)

		# Wake host delay - Time in seconds between each host waking
		self.wake_host_delay = Config.getInt("energySaverHostWakeInterval",20)
		
		# Time to pause after the loop.
		# Idle is when we didn't do anything in the loop
		self.idle_loop_time = Config.getInt("energySaverIdleLoopDelay",20)
		self.busy_loop_time = Config.getInt("energySaverBusyLoopDelay",10)
		

config = ESConfig()

# 1.0 means full load, 90% reset over 10 minutes
load_avg = 1.0
loop_run = 0

# Automatically setup the AB_manager service record
# the hostservice record for our host, and enable it
# if no other hosts are enabled for this service
service = Service.ensureServiceExists('AB_energy_saver')
hostService = service.byHost(Host.currentHost(),True)
hostService.enableUnique()

def wake_on_lan(macaddress, ipaddr, port):
	""" Switches on remote computers using WOL. """
	
	macaddress = str.strip(macaddress)	
	# Check macaddress format and try to compensate.
	if len(macaddress) == 12:
		pass
	elif len(macaddress) == 12 + 5:
		sep = macaddress[2]
		macaddress = macaddress.replace(sep, '')
	else:
		raise ValueError('Incorrect MAC address format: "' + macaddress + '"')
	
	pkt = ''
	for i in range(0,12,2):
		val = macaddress[i:i+2]
		pkt += chr(int(val,16))
	
	pkt = chr(0xFF)*6 + ''.join(pkt*16)
	
	raddr = socket.gethostbyname(ipaddr)
	proto = socket.getprotobyname('udp')
	them = (raddr, port)
	
	# Broadcast it to the LAN.
	sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, proto)
	sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
	print "sending magic packet to %s:%s with %s"%(ipaddr, port, macaddress)
	sock.sendto(pkt, them) #0, them)
	sock.close()

def wake_host(hostStatus):
	host = hostStatus.host()
	print "Waking up", host.name()
	interfaces = host.hostInterfaces()
	# Wake up the host
	for i in interfaces:
		try:
			wake_on_lan(str(i.mac()), '255.255.255.255', 9)
		except ValueError, ve:
			print "Host " + host.name() + " had invalid mac address: " + str(ve)
	hostStatus.setSlaveStatus( 'waking' )
	hostStatus.commit()
	
def get_stats():
	ret = {}
	q = Database.current().exec_( "select count(*), slavestatus from hoststatus where online=1 and slavestatus in('ready','copy','busy','offline') group by slavestatus" )
	print "get_stats:"
	while q.next():
		key = str(q.value(1).toString())
		val = q.value(0).toInt()[0]
		print key, val
		ret[key] = val
	for s in ['ready','copy','busy','offline']:
		if not ret.has_key(s):
			ret[s] = 0
			print s, 0
	return ret


while 1:
	hostService.pulse()
	service.reload()
	config.update()
	
	loop_time = config.idle_loop_time
	
	last_host_wake_time = QDateTime.currentDateTime().addSecs(-config.wake_host_delay)
	last_host_sleep_time = QDateTime.currentDateTime().addSecs(-config.sleep_host_delay)
	
	if service.enabled() and hostService.enabled():
		stats = get_stats()
		busy = stats['copy'] + stats['busy']
		idle = stats['ready']
		current = busy + idle
		load = busy / float(current)
		load_avg_mul = min(config.loop_time / float(config.load_avg_reset_time * 60), 1.0)
		if loop_run > 0:
			load_avg = load * load_avg_mul + load_avg * (1.0 - load_avg_mul)
		else:
			load_avg = load
		print "Awake: ", current
		print "Load:", load
		print "Load Avg:", load_avg
		
		# Keep sending WOL packets to waking hosts, in case
		# they didn't get the previous ones(they may have still
		# been shuting down, etc...)
		for hs in HostStatus.select( "slavestatus='waking'" ):
			wake_host(hs)
			
		# Only consider putting hosts to sleep if we have more idle hosts
		# than the buffer AND if load is either staying the same or decreasing
		if load_avg < 1.0 - config.percent_buffer and load <= load_avg:
			can_sleep = int((1.0 - config.percent_buffer - load_avg) * current)
			print can_sleep, "hosts can sleep"
			if can_sleep > 0:
				hostStatuses = HostStatus.select("WHERE now() - laststatuschange > '" + str(config.time_before_idle) + " seconds'::interval and slavestatus='ready' and fkeyhost in (SELECT keyhost FROM Host WHERE allowsleep=true AND wakeonlan=true)").sorted( "laststatuschange" )
				index = 0
				while can_sleep > index and index < hostStatuses.size() and last_host_sleep_time.secsTo(QDateTime.currentDateTime()) >= config.sleep_host_delay:
					hostStatus = hostStatuses[index]
					print "Putting host", hostStatus.host().name(), "to sleep"
					hostStatus.setSlaveStatus( 'sleep' )
					hostStatus.commit()
					index += 1
					last_host_sleep_time = QDateTime.currentDateTime()
				if index > 0:
					loop_time = config.busy_loop_time
		
		elif load > 1.0 - config.percent_buffer and busy * config.percent_buffer > idle:
			can_wake = busy * config.percent_buffer - idle
			print can_wake, "hosts can wake"
			if can_wake > 0:
				hostStatuses = HostStatus.select("slavestatus='sleeping'")
				index = 0
				while can_wake > index and index < hostStatuses.size() and last_host_wake_time.secsTo(QDateTime.currentDateTime()) >= config.wake_host_delay:
					hostStatus = hostStatuses[index]
					wake_host(hostStatus)
					index += 1
					last_host_wake_time = QDateTime.currentDateTime()
				if index > 0:
					loop_time = config.busy_loop_time
		else:
			print "Nothing to do"
		print "Sleeping for %i seconds" % (loop_time)
		loop_run += 1
	time.sleep(loop_time)
	
