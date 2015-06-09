
import sys
from blur.build import *

path = os.path.dirname(os.path.abspath(__file__))
sippath = os.path.join(path,'sipStone')

try:
    os.mkdir(sippath)
except: pass

# Install the libraries
if sys.platform != 'win32':
	LibInstallTarget("stoneinstall",path,"stone","/usr/lib/")

# Python module targets
SipTarget("pystone",path,False,None,["sip:install","pyqt:install","stone"])
sst = SipTarget("pystonestatic",path,True,None,["sip:install","pyqt:install","stone"])

# Create the main qmake target
QMakeTarget("stone",path,"stone.pro")

if sys.platform=="linux2":
	rpm = RPMTarget('stonerpm','libstone',path,'../../../rpm/spec/stone.spec.template','1.0',["siprpm", "pyqtrpm", "stone","pystone","stonegui","pystonegui"])
#	rpm.pre_deps = ["pyqtrpm"]

#	pyrpm = RPMTarget('pystonerpm','pystone',path,'../../../rpm/spec/pystone.spec.template','1.0')

if __name__ == "__main__":
	build()
