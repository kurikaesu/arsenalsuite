import sys
from blur.build import *

path = os.path.dirname(os.path.abspath(__file__))

QMakeTarget("rlaplugin",path,"rla.pro")
