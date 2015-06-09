
from blur.build import *
import os

pre_deps = ["stone"]
post_deps = []

path = os.path.dirname(os.path.abspath(__file__))
rev_path = os.path.join(path,'../..')

# Here is the static command for running classmaker to generate the classes
classmakercmd = 'classmaker'
if sys.platform == 'win32':
        classmakercmd = 'classmaker.exe'

# Run using cmd in path, unless we are inside the tree
if os.path.isfile(os.path.join(path,'../../apps/classmaker/',classmakercmd)):
        if sys.platform != 'win32':
                classmakercmd = './' + classmakercmd
        classmakercmd = 'cd ../../apps/classmaker && ' + classmakercmd

bachclassgen = StaticTarget("classgen",path,classmakercmd + " -s " + path + "/schema.xml -o " + path,["classmaker"],shell=True)

ini = IniConfigTarget("bachini",path,'bach.ini.template','bach.ini')
svn = WCRevTarget("bachsvnrev",path,rev_path,"core/svnrev-template.h","core/svnrev.h")
svnnsi = WCRevTarget("bachsvnrevnsi",path,rev_path,"bach-svnrev-template.nsi","bach-svnrev.nsi")
svntxt = WCRevTarget("bachsvnrevtxt",path,rev_path,"bach_version_template.txt","bach_version.txt");
nsi = NSISTarget("bach_installer",path,"bach.nsi")
QMakeTarget("bach",path,"bach.pro",
    ["stone","stonegui",bachclassgen,svnnsi,svntxt,svn,ini],[nsi])

if __name__ == "__main__":
	build()
