
from blur.build import *
import os, sys

path = os.path.dirname(os.path.abspath(__file__))
sip_path = os.path.join(path,'sipBurner')
rev_path = os.path.join(path,'../..')

try:
    os.mkdir(sip_path)
except: pass

instPrefix = ""
destDir = ""

if "DESTDIR" in os.environ:
    destDir = os.environ["DESTDIR"]
elif sys.platform=="win32":
	destDir = "C:/"
	
if sys.platform=="linux2":
    instPrefix = destDir + "/etc/ab/"
elif sys.platform=="win32":
    instPrefix = destDir + "arsenalsuite/burner/"
   
ini = IniConfigTarget("burnerini",path,'burner.ini.template','burner.ini',instPrefix)
kt = KillTarget("burnerkill", path, ["abpsmon.exe","burner.exe"])
nsi = NSISTarget("burnerinstaller",path,"burner.nsi")
nsi.pre_deps = ['burnerkill']
st = SipTarget("pyburner",path)
sst = SipTarget("pyburnerstatic",path,True)
#abgui = QMakeTarget("abgui","apps/assburner", "abgui.pro", ["stonegui","classes",svn])

# Use Static python modules on windows
deps = None
if sys.platform == 'win32':
	deps = ["sipstatic","pystonestatic","pyclassesstatic","classes","pyburnerstatic",ini]
else:
	deps = ["sipstatic","pystone","pyclasses","classes","pyburner",ini]

assburner = QMakeTarget("burner",path, "burner.pro", deps)
abpsmon = QMakeTarget("abpsmon",os.path.join(path,'psmon'), "psmon.pro", ["stonegui","classes",ini])
Target("burner",path, [abpsmon],[nsi])

if sys.platform=="linux2":
	rpm = RPMTarget('burnerrpm','burner',path,'../../../rpm/spec/burner.spec.template','1.0',["classesrpm","burner","pyburner","abpsmon"])
#	rpm.pre_deps = ['stoneguirpm','classesrpm','stonerpm']

if __name__ == "__main__":
	build()
