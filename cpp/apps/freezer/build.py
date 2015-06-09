
from blur.build import *
import os, sys

path = os.path.dirname(os.path.abspath(__file__))
rev_path = os.path.join(path,'../..')

instPrefix = ""
destDir = ""

if "DESTDIR" in os.environ:
    destDir = os.environ["DESTDIR"]
elif sys.platform=='win32':
	destDir = "c:/"
	
if sys.platform=="linux2":
    instPrefix = destDir + "/etc/ab/"
else:
	instPrefix = destDir + "/arsenalsuite/freezer/"
	
ini = IniConfigTarget("freezerini",path,'freezer.ini.template','freezer.ini',instPrefix)
kt = KillTarget("freezerkill", path, ["af.exe"])
nsi = NSISTarget("freezer_installer",path,"freezer.nsi")
nsi.pre_deps = ['freezerkill']

# Use Static python modules on windows
deps = None
if sys.platform == 'win32':
	deps = ["sipstatic","pystonestatic","pystoneguistatic","pyclassesstatic","pyclassesuistatic","classes","libfreezer","pyfreezerstatic","pyabsubmitstatic",ini]
else:
	deps = ["sipstatic","pystone","pystonegui","pyclasses","pyclassesui","classes","libfreezer","pyfreezer",ini]

QMakeTarget("freezer",path,"freezer.pro",deps,[nsi])

#if sys.platform=="linux2":
#	rpm = RPMTarget('freezerrpm','freezer',path,'../../../rpm/spec/freezer.spec.template','1.0')

if __name__ == "__main__":
	build()
