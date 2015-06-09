
from blur.Stone import *
from blur.Classes import *
from blur.Freezer import *
from PyQt4.QtCore import *
from PyQt4.QtSql import *
import traceback, os
import subprocess

class PowerDownBladeViewerPlugin(HostViewerPlugin):
    def __init__(self):
        HostViewerPlugin.__init__(self)

    def name(self):
        return QString("iLo - Power off blade")

    def icon(self):
        return QString(":/images/host_offline")

    def view(self, hostList):
        cmd = ["/drd/software/int/sys/renderops/rpower", "off"]
        hosts = []
        for host in hostList:
            if host.name().startsWith("c0"):
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
            HostViewerFactory.registerPlugin( PowerDownBladeViewerPlugin() )
            break

