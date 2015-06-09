
from blur.build import *
import sys
import os

path 		= os.path.dirname(os.path.abspath(__file__))

svnnsi 		= WCRevTarget( 		"pythondotnetsvnrevnsi",	path,	path,		"pythondotnet-svnrev-template.nsi",		"pythondotnet-svnrev.nsi")
nsi 		= NSISTarget( 		"pythondotnet_installer",	path,				"pythondotnet.nsi")

Target( 'pythondotnet', path, [svnnsi], [nsi] )

if __name__ == "__main__":
	build()