
import os
from blur.build import *

path = os.path.dirname(os.path.abspath(__file__))
rev_path = os.path.join(path,'..')

svnnsi = WCRevTarget("libs_installersvnrevnsi",path,"../..","blur-svnrev-template.nsi","blur-svnrev.nsi")
svntxt = WCRevTarget("libs_installersvnrevtxt",path,rev_path,"blur_libs_version_template.txt","blur_libs_version.txt")
NSISTarget( "libs_installer", path, "blur_libs.nsi", [svnnsi,svntxt] )
