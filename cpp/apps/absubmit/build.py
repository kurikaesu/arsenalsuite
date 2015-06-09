
import os, sys
from blur.build import *

path = os.path.dirname(os.path.abspath(__file__))
rev_path = os.path.join(path,'../..')

ini = IniConfigTarget("absubmitini",path,'absubmit.ini.template','absubmit.ini')

nsi = NSISTarget("absubmit_installer",path,"absubmit.nsi")
All_Targets.append(QMakeTarget("absubmit",path,"absubmit.pro",
    ["stone","classes","libabsubmit",ini],[nsi]))

#if sys.platform=="linux2":
#	rpm = RPMTarget('absubmitrpm','absubmit',path,'../../../rpm/spec/absubmit.spec.template','1.0')
#	rpm.pre_deps = ['libabsubmitrpm']

if __name__ == "__main__":
	build()
