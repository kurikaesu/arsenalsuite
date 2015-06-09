
import os
from blur.build import *

path = os.path.dirname(os.path.abspath(__file__))

NSISTarget("qt_installer",path,"qt4.nsi",["qimageioplugins","qscintilla"])
NSISTarget("qt_dev_installer",path,"qt4-dev.nsi")
