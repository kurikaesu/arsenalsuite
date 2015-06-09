
from blur.Stone import *
from blur.Classes import *
from blur.Freezer import *
from PyQt4.QtCore import *
from PyQt4.QtSql import *
import traceback, os
import subprocess, re

class RvViewerPlugin(JobViewerPlugin):
    def __init__(self):
        JobViewerPlugin.__init__(self)

    def name(self):
        return QString("rv")

    def icon(self):
        return QString("images/rv2.png")

    def view(self, jobList):
        for job in jobList:
            envdict = {}
            env_lines = str(job.environment().environment()).splitlines()
            rv_path = ""
            for line in env_lines:
                values = line.split('=', 1)
                if len(values) >= 2:

                    maxLength = 4100

                    if len(values[1]) > maxLength:
                        lastPos = values[1][:maxLength].find(':')
                        envdict[values[0]] = values[1][:lastPos]
                        if values[0] == "PATH":
                            if values[1][:lastPos].find("rv") == -1:
                                rv_path = re.search("[\w/\.-]+rv[\w/\.-]+", values[1]).group(0)
                                rv_path.rstrip(':')
                                envdict[values[0]] += ":" + rv_path

                            if values[1][:lastPos].find("/usr/bin") == -1:
                                envdict[values[0]] += ":/usr/bin"
                    else:
                        envdict[values[0]] = values[1]

            subprocess.Popen(["rv", "-l", job.outputPath().replace("..",".#.")], env=envdict)

JobViewerFactory.registerPlugin( RvViewerPlugin() )
