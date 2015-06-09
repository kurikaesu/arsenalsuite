import sys
from blur.build import *

path = os.path.dirname(os.path.abspath(__file__))

QMakeTarget("sgiplugin",path,"sgi.pro")
