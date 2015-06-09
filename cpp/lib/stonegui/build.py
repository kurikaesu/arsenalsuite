
from blur.build import *
import os, sys

path = os.path.dirname(os.path.abspath(__file__))
sippath = os.path.join(path,'sipStonegui')

post_deps = []
# Install the libraries
if sys.platform != 'win32':
	LibInstallTarget("stoneguiinstall",path,"stonegui","/usr/lib/")

try:
	os.mkdir(sippath)
except:	pass

# Python module targets, both depend on classes
pc = SipTarget("pystonegui",path)
pc.pre_deps = ["stonegui","pystone:install"]
sst = SipTarget("pystoneguistatic",path,True)
sst.pre_deps = ["stonegui"]

Target = QMakeTarget("stonegui",path,"stonegui.pro",["stone"],post_deps)
StaticTarget = QMakeTarget("stoneguistatic",path,"stonegui.pro",["stone"],[],True)

#if sys.platform=="linux2":
#	rpm = RPMTarget("stoneguirpm",'blur-stonegui',path,'../../../rpm/spec/stonegui.spec.template','1.0')
#	pyrpm = RPMTarget('pystoneguirpm','pystonegui',path,'../../../rpm/spec/pystonegui.spec.template','1.0')

if __name__ == "__main__":
	build()