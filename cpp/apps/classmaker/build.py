
import sys
from blur.build import *

path = os.path.dirname(os.path.abspath(__file__))
lib_path = os.path.join(path,'../../lib')
rpm_path = os.path.join(path,'../../../rpm/spec')
rev_path = os.path.join(path,'../..')

ini = IniConfigTarget("classmakerini",path,'classmaker.ini.template','classmaker.ini')
# Stone is a pre dependancy
# Build classmaker with qmake
# Then Run classmaker to generate the classes
pre_deps = ["stone","stonegui",ini]
if sys.platform == 'win32':
	pre_deps.append(CopyTarget("stoneclasseslibcopy", path, os.path.join(lib_path,'stone/stone.dll'), "stone.dll"))
	pre_deps.append(CopyTarget("stoneguiclasseslibcopy", path, os.path.join(lib_path,'stonegui/stonegui.dll'), "stonegui.dll"))

#svn = WCRevTarget("classmakersvnrev",path,rev_path,"src/svnrev-template.h","src/svnrev.h")
#svnnsi = WCRevTarget("classmakersvnrevnsi",path,rev_path,"classmaker-svnrev-template.nsi","classmaker-svnrev.nsi")
#svntxt = WCRevTarget("classmakersvnrevtxt",path,rev_path,"classmaker_version_template.txt","classmaker_version.txt");
kt = KillTarget("classmakerkill", path, ["classmaker.exe"])
nsi = NSISTarget("classmaker_installer",path,"classmaker.nsi")
nsi.pre_deps = ["classmakerkill"]

QMakeTarget("classmaker",path,"classmaker.pro", pre_deps,[nsi])

#if sys.platform=="linux2":
#      rpm = RPMTarget('classmakerrpm','blur-classmaker',path,os.path.join(rpm_path,'classmaker.spec.template'),'1.0')

if __name__ == "__main__":
	build()
