
import os
from blur.build import *

path = os.path.dirname(os.path.abspath(__file__))

#svn = WCRevTarget("assfreezersvnrev","apps/assfreezer","../..","src/svnrev-template.h","src/svnrev.h")
#svnnsi = WCRevTarget("assfreezersvnrevnsi","apps/assfreezer","../..","assfreezer-svnrev-template.nsi","assfreezer-svnrev.nsi")
#nsi = NSISTarget("assfreezer_installer","apps/assfreezer","assfreezer.nsi")
All_Targets.append(QMakeTarget("rum",path,"rum.pro",["stone"]))
