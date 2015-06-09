#!/usr/bin/python2.5

from PyQt4.QtCore import *
from PyQt4.QtSql import *
from PyQt4.Qt import *
from blur.Stone import *
from Bach import *
import sys, os

app = QCoreApplication.instance()
if app is None:
  try:
    args = sys.argv
  except:
    args = []
  app = QCoreApplication(args)

initConfig("bach_%s.ini" % os.environ['DRD_JOB'], "/tmp/pybach.log")

initStone(sys.argv)
bach_loader()

