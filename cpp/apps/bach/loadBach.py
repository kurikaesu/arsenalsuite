#!/usr/bin/python2.5

import os
import sys
import subprocess

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.Qt import *

def checkPermissions():
    if not os.environ.has_key("BACH_USERFILE"):
        return False, "Don't have a USERFILE defined!"

    userfile = os.environ["BACH_USERFILE"]
    if userfile.strip() == "":
        return False, "USERFILE definition is empty!"

    if not os.path.isfile(userfile):
        return False, "USERFILE is not a file [%s]!"%userfile

    fp = file(userfile,"rt")
    lines = fp.readlines()
    if len(lines) == 0:
        return False, "USERFILE is empty [%s]!"%userfile

    lines = [ l.strip() for l in lines ]

    if not os.environ.has_key("USER"):
        return False, "User not defined?!?!"

    username = os.environ["USER"]
    if username.strip() == "":
        return False, "User is empty!"

    if username not in lines:
        return False, "You do not have permissions to run Bach in editor mode.\nPlease see Rachael Byrne or Andrew Jackson."

    if not os.environ.has_key("BACH_CMDLINE"):
        return False, "There is no BACH_CMDLINE defined!"

    cmdline = os.environ["BACH_CMDLINE"]
    if cmdline.strip() == "":
        return False, "BACH_CMDLINE is empty!"

    return True,cmdline

stat,ret = checkPermissions()
if stat is not True:
    QApplication(sys.argv)
    QMessageBox.critical(None,"Bach","Can't load Bach\n%s"%ret)
    sys.exit(-1)

subprocess.Popen(ret.split())
