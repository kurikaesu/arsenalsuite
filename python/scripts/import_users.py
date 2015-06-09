import initab
import sys
from blur.Classes import *

lines = sys.stdin.readlines()
for line in lines:
	#barry.robison:*:1000:1002:Barry Robison:/drd/users/barry.robison:/bin/bash
	fields = line.split(':')
	username = fields[0]
	uid = fields[2]
	gid = fields[3]
	name = fields[4]
	homedir = fields[5]
	shell = fields[6]
	user = Employee.recordByUserName(username)
	if user.isRecord():
		print "user %s exists" % username
	else:
		print "create user %s" % username
		user = Employee()
		user.setName(username)
		user.setLogon(username)
		user.setUid(int(uid))
		user.setGid(int(gid))
		user.setHomeDir(homedir)
		user.setShell(shell)
		user.commit()

