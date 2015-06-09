#!/usr/bin/python

from PyQt4.QtCore import *
from blur.Stone import *
from blur.Classes import *
import sys

def hostGroups():
	return HostGroup.select("private is NULL or private=false")

def hostGroupsByUser(userName):
	user = User.recordByUserName(userName)
	return HostGroup.select("fkeyusr=%i" % user.key())

def hostsByJobType(jobTypeName):
	jobType = JobType.recordByName(jobTypeName)
	if jobType.isRecord():
		return jobType.service().hostServices().hosts().sorted("name")
	return HostList()

def hostsByGroupName(groupName):
	group = HostGroup.recordByName(groupName)
	return group.hosts().sorted("name")

usage_text = """
python hostlist_dump.py COMMAND OUTPUT_FILE [OPTIONS]

Outputs a text file with each host name and status or host group name.

COMMANDS - 
	hostGroupNames [USER_NAME]
		Returns the names of all host groups, username is optional
	hostsByJobType JOB_TYPE
		Returns all the host names and statuses that can handle JOB_TYPE
	hostsByGroupName HOST_GROUP_NAME [JOB_TYPE]
		Returns all the host names and statuses that exist, filtered to only include hosts that can handle JOB_TYPE, if jobtype is present
		
Output format for hostsByJobType and hostsByGroupName
Each line consists of
Host_Name Status

Output format for hostGroupNames
Each line consists of
Host_Group_Name

Examples:
python hostlist_dump.py hostGroupNames C:/temp/host_group_names.txt
python hostlist_dump.py hostGroupNames C:/temp/host_group_names.txt newellm
python hostlist_dump.py hostsByJobType C:/temp/max9_hosts.txt Max9
python hostlist_dump.py hostsByGroupName 'All Ultrons' Max8
"""

def usage():
	print usage_text

def writeHosts(fileh, hosts):
	statuses = HostStatus.select("fkeyhost in (" + hosts.keyString() + ")")
	for host in hosts:
		fileh.write( host.name() + '; ' + host.hostStatus().slaveStatus() + '; ' + str(host.memory()) + '; ' + str(host.mhz()) + '; ' + host.description().replace(';',',') + '\n' )

def writeHostGroups(fileh, hostGroups):
	for hostGroup in hostGroups:
		groupType = 'Static'
		if DynamicHostGroup(hostGroup).isRecord(): groupType = 'Dynamic'
		fileh.write( "%s; %s; %s\n" % (hostGroup.name(), hostGroup.user().displayName(), groupType) )

if __name__ == "__main__":
	app = QCoreApplication(sys.argv)
	if sys.platform == 'win32':
		initConfig('c:/blur/absubmit/absubmit.ini')
	else:
		initConfig('/etc/db.ini')
	blurqt_loader()
	if len(sys.argv) < 3:
		usage()
		sys.exit(1)
	command = sys.argv[1].lower()
	if not command in ['hostgroupnames','hostsbyjobtype','hostsbygroupname']:
		print 'Unexpected command: ', sys.argv[1], '\n\n'
		sys.exit(1)
	outputFilePath = sys.argv[2]
	try:
		outputFile = open(outputFilePath,'w')
	except:
		print "Unable to open output file for writing"
		sys.exit(1)
	if command=='hostgroupnames':
		hgl = HostGroupList()
		if len(sys.argv) >= 4:
			hgl = hostGroupsByUser(sys.argv[3])
		else:
			hgl = hostGroups()
		writeHostGroups( outputFile, hgl )
	elif command == 'hostsbyjobtype':
		if len(sys.argv) < 4:
			print "Missing JobType option\n\n"
			usage()
			sys.exit(1)
		writeHosts(outputFile,hostsByJobType( sys.argv[3] ))
	elif command == 'hostsbygroupname':
		if len(sys.argv) < 4:
			print "Missing JobType option\n\n"
			usage()
			sys.exit(1)
		hosts = hostsByGroupName(sys.argv[3])
		if len(sys.argv) > 4:
			hosts &= hostsByJobType(sys.argv[4])
		writeHosts(outputFile,hosts)
	sys.exit(0)
