
from blur.build import *
import os

path = os.path.dirname(os.path.abspath(__file__))
rev_path = os.path.join(path,'../..')

svn = WCRevTarget("snafusvnrev",path,rev_path,"src/svnrev-template.h","src/svnrev.h")
svnnsi = WCRevTarget("snafusvnrevnsi",path,rev_path,"snafu-svnrev-template.nsi","snafu-svnrev.nsi")
svntxt = WCRevTarget("snafusvnrevtxt",path,rev_path,"snafu_version_template.txt","snafu_version.txt");
nsi = NSISTarget("snafu_installer",path,"snafu.nsi")
All_Targets.append(QMakeTarget("snafu",path,"snafu.pro",
    ["stone","classes","libsnafu",svnnsi,svntxt,svn],[nsi]))

#rpm = RPMTarget('assfreezerrpm','assfreezer',path,'../../../rpm/spec/assfreezer.spec.template','1.0')
#All_Targets.append(rpm)

if __name__ == "__main__":
	build()
