
import os
from blur.build import *

path = os.path.dirname(os.path.abspath(__file__))
sippath = os.path.join(path,'sipFreezer')

try:
    os.mkdir(sippath)
except: pass

rev_path = os.path.join(path,'../..')

svn = WCRevTarget("libfreezersvnrev",path,rev_path,"include/svnrev-template.h","include/svnrev.h")

# Python module target
pc = SipTarget("pyfreezer",path)
pc.pre_deps = ["libfreezer","pyclasses:install","pyabsubmit"]

pcs = SipTarget("pyfreezerstatic",path,True)
pcs.pre_deps = ["libfreezer","pyclassesstatic:install","pyabsubmitstatic"]

QMakeTarget("libfreezer",path,"libfreezer.pro",["classes","classesui","libabsubmit",svn])
#QMakeTarget("libassfreezerstatic",path,"libassfreezer.pro",["stonestatic","stoneguistatic","classesuistatic","libabsubmit"],[],True)

if sys.platform=="linux2":
	rpm = RPMTarget('libfreezerrpm','libfreezer',path,'../../../rpm/spec/libfreezer.spec.template','1.0',["classesrpm","libfreezer","freezer","pyfreezer"])
	rpm.pre_deps = ["libabsubmitrpm"]


if __name__ == "__main__":
	build()
