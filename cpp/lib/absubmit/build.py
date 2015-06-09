
import os, sys
from blur.build import *

path = os.path.dirname(os.path.abspath(__file__))
QMakeTarget("libabsubmit",path,"libabsubmit.pro", ["stone","classes"],[])

if sys.platform=="linux2":
	rpm = RPMTarget('absubmitrpm','absubmit',path,'../../../rpm/spec/absubmit.spec.template','1.0',["absubmit","pyabsubmit"])
#	rpm = RPMTarget('libabsubmitrpm','libabsubmit',path,'../../../rpm/spec/libabsubmit.spec.template','1.0',["absubmit","pyabsubmit"])
#	rpm.pre_deps = ['classesrpm','stonerpm']

#	pyrpm = RPMTarget('pyabsubmitrpm','pyabsubmit',path,'../../../rpm/spec/pyabsubmit.spec.template','1.0')
#	pyrpm.pre_deps = ['pystonerpm','pyclassesrpm','libabsubmitrpm']

# Python module target
pc = SipTarget("pyabsubmit",path)
if os.name == 'nt':
    pc.pre_deps = ["libabsubmit","pyclassesstatic:install"]
else:
    pc.pre_deps = ["libabsubmit","pyclasses:install"]

pcs = SipTarget("pyabsubmitstatic",path,True)
pcs.pre_deps = ["libabsubmit", "pyclassesstatic:install"]

if __name__ == "__main__":
	build()
