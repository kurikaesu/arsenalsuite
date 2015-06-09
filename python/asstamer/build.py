
from blur.build import *
import os, sys

path = os.path.dirname(os.path.abspath(__file__))
rev_path = os.path.join(path,'../..')

svn = WCRevTarget("asstamersvnrev",path,rev_path,"svnrev-template.py","svnrev.py")
svnnsi = WCRevTarget("asstamersvnrevnsi",path,rev_path,"asstamer-svnrev-template.nsi","asstamer-svnrev.nsi")
svntxt = WCRevTarget("asstamersvnrevtxt",path,rev_path,"asstamer_version_template.txt","asstamer_version.txt");

post_deps = []
if sys.platform=='win32':
	post_deps.append( NSISTarget("asstamer_installer",path,"asstamer.nsi",[svnnsi,svntxt]) )

All_Targets.append(Target("asstamer",path,[svn],post_deps))

if __name__ == "__main__":
	build()
