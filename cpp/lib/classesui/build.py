
from blur.build import *

path = os.path.dirname(os.path.abspath(__file__))
sippath = os.path.join(path,'sipClassesui')

post_deps = []

# Install the libraries
if sys.platform != 'win32':
	LibInstallTarget("classesuiinstall",path,"classesui","/usr/lib/")

try:
	os.mkdir(sippath)
except: pass

# Python module targets, both depend on classes
pc = SipTarget("pyclassesui",path)
pc.pre_deps = ["classesui","pyclasses:install","pystonegui:install"]
sst = SipTarget("pyclassesuistatic",path,True)
sst.pre_deps = ["classesui"]

Target = QMakeTarget("classesui",path,"classesui.pro",["stonegui","classes"],post_deps)
StaticTarget = QMakeTarget("classesuistatic",path,"classesui.pro",["classesui"],[],True)

#if sys.platform=="linux2":
#      rpm = RPMTarget('classesuirpm','libclassesui',path,'../../../rpm/spec/classesui.spec.template','1.0')
#      pyrpm = RPMTarget('pyclassesuirpm','pyclassesui',path,'../../../rpm/spec/pyclassesui.spec.template','1.0')

if __name__ == "__main__":
	build()
