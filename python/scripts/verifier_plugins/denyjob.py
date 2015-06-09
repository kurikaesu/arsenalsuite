
from PyQt4.QtCore import *
from PyQt4.QtSql import *
from blur.Stone import *
from blur.Classes import *

from verifier_plugin_factory import *

def doThis(job):
    if( job.name().contains( "denyMePlease" ) ):
        job.setStatus("denied")
        job.commit()
        return False
    return True

VerifierPluginFactory().registerPlugin("denyJob", doThis)

