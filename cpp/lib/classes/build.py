
import sys
from blur.build import *

path = os.path.dirname(os.path.abspath(__file__))
sippath = os.path.join(path,'sipClasses')

try:
	os.mkdir(sippath)
except:	pass

pre_deps = ["stone"]
post_deps = []

classgen = ClassGenTarget("classgen",path,path + "/schema.xml", ["classmaker"])
pre_deps.append(classgen)

# Python module targets, both depend on classes
pc = SipTarget("pyclasses",path)
pc.pre_deps = [classgen,"classes","pystone:install"]
sst = SipTarget("pyclassesstatic",path,True)

sst.pre_deps = [classgen,"classes"]

# Install the libraries
if sys.platform != 'win32':
	LibInstallTarget("classesinstall",path,"classes","/usr/lib/")

QMakeTarget("classes",path,"classes.pro",pre_deps,post_deps)

# Create versioned dll and lib file
svnpri = WCRevTarget("classeslibsvnrevpri",path,"../..","svnrev-template.pri","svnrev.pri")
#post_deps.append(LibVersionTarget("classeslibversion","lib/classes","../..","classes"))
sv = QMakeTarget("classesversioned",path,"classes.pro",pre_deps + [svnpri],post_deps)
sv.Defines = ["versioned"]

if sys.platform=="linux2":
	rpm = RPMTarget('classesrpm','blur-classes',path,'../../../rpm/spec/classes.spec.template','1.0',["stonerpm", "classes","classesui","classmaker","pyclasses","pyclassesui"])
#	pyrpm = RPMTarget('pyclassesrpm','pyclasses',path,'../../../rpm/spec/pyclasses.spec.template','1.0')

if __name__ == "__main__":
	build()
