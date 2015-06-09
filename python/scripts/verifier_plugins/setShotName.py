
from PyQt4.QtCore import *
from PyQt4.QtSql import *
from blur.Stone import *
from blur.Classes import *

from verifier_plugin_factory import *

def doThis(job):
    print "setShotName - checking %s" % job.name()

    if( job.shotName().isEmpty() ):
        # try to suss shot name from the environment
        rx = QRegExp("DRD_SHOT=(\w+)")
        if rx.indexIn(job.environment().environment()) >= 0:
            shotName = rx.cap(1)
            job.setShotName(shotName)
            job.commit()

    return True

VerifierPluginFactory().registerPlugin("setShotName", doThis)

