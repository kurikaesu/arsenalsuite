
import sys
from blur.build import *

path = os.path.dirname(os.path.abspath(__file__))
sippath = os.path.join(path,'sipEpa')

try:
    os.mkdir(sippath)
except: pass

# Install the libraries
if sys.platform != 'win32':
	LibInstallTarget("epainstall",path,"epa","/usr/lib/")

# Python module targets
SipTarget("pyepa",path,False,None,["sip:install","pyqt:install","epa"])
sst = SipTarget("pyepastatic",path,True)

if sys.platform=='win32':
	lib = 'pyEpa.lib'
	srclib = 'Epa.lib'
	if sys.argv.count("debug"):
		lib = 'pyEpa.lib'
		srclib = 'Epa_d.lib'
	sst.post_deps = [CopyTarget("pyepacopy",sippath,srclib,lib)]
else:
	sst.post_deps = [CopyTarget("pyepacopy",sippath,"libEpa.a","libpyEpa.a")]

# Create the main qmake target
QMakeTarget("epa",path,"epa.pro")

# Create versioned dll and lib file
svnpri = WCRevTarget("epalibsvnrevpri",path,"../..","svnrev-template.pri","svnrev.pri")
#post_deps.append(LibVersionTarget("stonelibversion","lib/stone","../..","stone"))

#sv = QMakeTarget("stoneversioned",path,"stone.pro",[],[svnpri])
#sv.Defines = ["versioned"]

rpm = RPMTarget('eparpm','libepa',path,'../../../rpm/spec/epa.spec.template','1.0')
rpm.pre_deps = ["pyqtrpm"]

pyrpm = RPMTarget('pyeparpm','pyepa',path,'../../../rpm/spec/pyepa.spec.template','1.0')

if __name__ == "__main__":
	build()
