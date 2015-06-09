from PyQt4.QtCore import *
from PyQt4.QtSql import *
from blur.Stone import *
from blur.Classes import *
import sys, os
import traceback

app = QCoreApplication.instance()
if app is None:
  try:
    args = sys.argv
  except:
    args = []
  app = QCoreApplication(args)

if sys.platform == 'win32':
  initConfig('t:/drd/software/ext/ab/lin64/7852/ab.ini')
else:
  initConfig(os.environ['ABSUBMIT']+'/ab.ini')

initStone(sys.argv)
classes_loader()

