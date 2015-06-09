#!/usr/bin/python2.5

import sys
import time
import os

import nuke

def launchSubmit():
	print("nukeStub(): launch submitter dialog")
	submitCmd = "/drd/software/int/bin/launcher.sh -p %s -d %s --launchBlocking farm -o EPA_CMDLINE python2.5 --arg '$ABSUBMIT/nukesubmit/nuke2AB.py'" % (os.environ['DRD_JOB'], os.environ['DRD_DEPT'])
	# root.name holds the path to the nuke script
	submitCmd += " %s" % nuke.value("root.name")
	submitCmd += " %s" % nuke.Root.firstFrame(nuke.root())
	submitCmd += " %s" % nuke.Root.lastFrame(nuke.root())
	writeNodes = [i for i in nuke.allNodes() if i.Class() == "Write"]
	for i in writeNodes:
		submitCmd += " %s %s" % (i['name'].value(), nuke.filename(i))

	print( "nukeStub(): %s" % submitCmd )
	os.system(submitCmd)

menubar = nuke.menu("Nuke")
m = menubar.addMenu("&Render")
m.addCommand("Submit to Farm", "nukeStub.launchSubmit()", "Up")

