
import os
from blur.build import *
from distutils import sysconfig

path = os.path.dirname(os.path.abspath(__file__))

# Replace revision numbers in the nsi template
#svnnsi = WCRevTarget("pyqt_modulessvnrevnsi",path,"../..","pyqt-svnrev-template.nsi","pyqt-svnrev.nsi")

# Create the nsi installer
pyqt_modules_installer = NSISTarget( "pyqt_modules_installer", path, "pyqt.nsi", [], makensis_extra_options = ['/DPYTHON_PATH=%s' % sysconfig.get_config_vars()['prefix'] ] )

Target( "pyqt_modules", path, ["pystone","pyclasses","pystonegui","pyclassesui","pyabsubmit"], [pyqt_modules_installer] )


