
import sys, os
from blur.build import *

path = os.path.dirname(os.path.abspath(__file__))

pre_deps = ["stone","stonegui"]

# Install the libraries
#if sys.platform != 'win32':
#	All_Targets.append(LibInstallTarget("classesinstall","lib/classes","classes","/usr/lib/"))

# Python module targets, both depend on classes
QMakeTarget("guiplugin",path,"guiplugin.pro",pre_deps)
