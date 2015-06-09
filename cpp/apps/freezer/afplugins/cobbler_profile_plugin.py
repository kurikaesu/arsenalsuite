
from blur.Stone import *
from blur.Classes import *
from blur.Freezer import *
from PyQt4.QtCore import *
from PyQt4.QtSql import *
import traceback, os
import subprocess

class CobblerProfileViewerPlugin(HostViewerPlugin):
    def __init__(self):
        HostViewerPlugin.__init__(self)

    def name(self):
        return QString("Switch Cobbler Profile")

    def icon(self):
        return QString("images/history.png")

    def view(self, hostList):
        cmd = ["/drd/software/ext/ab/cobblerPlugin/cobblerProfileSwitcherDialog.py"]
        hosts = []
        for host in hostList:
            hosts.append(str(host.name()))

        cmd.append(",".join(hosts))

        if len(hosts) > 0:
            print "%s" % " ".join(cmd)

            subprocess.Popen(cmd)

currentUser = User.currentUser()
if currentUser.userGroups().size() > 0:
    groupList = currentUser.userGroups()
    for userGroup in groupList:
        if userGroup.group().name() == "Admin" or userGroup.group().name() == "RenderOps":
            HostViewerFactory.registerPlugin( CobblerProfileViewerPlugin() )
            break

