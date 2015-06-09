
import os
from blur.build import *

path = os.path.dirname(os.path.abspath(__file__))
rev_path = os.path.join(path,'../..')

svnnsi = WCRevTarget("farmreportsvnrevnsi",path,rev_path,"farmreport-svnrev-template.nsi","farmreport-svnrev.nsi")
nsi = NSISTarget("farmreport_installer",path,"farmreport.nsi")
All_Targets.append(Target("farmreport",path,[svnnsi],[nsi]))

if __name__ == "__main__":
	build()
