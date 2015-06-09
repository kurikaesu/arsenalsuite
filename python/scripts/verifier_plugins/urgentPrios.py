
from PyQt4.QtCore import *
from PyQt4.QtSql import *
from blur.Stone import *
from blur.Classes import *

from verifier_plugin_factory import *

requiredShots = {}
requiredShots['30'] = [['21a_020', 'hf2-light'],
                      ['21a_040', 'hf2-light'],
                      ['04_060', 'hf2-light'],
                      ['04_080', 'hf2-light']]

def doThis(job):
    print "urgentPrios - checking %s" % job.name()

    if( job.user().name() == "tankadmin" ):
        return True

    jobName = job.name()
    for key, pairs in requiredShots.iteritems():
        for pair in pairs:
            if ( jobName.contains(pair[0]) and job.project().shortName() == pair[1] ):
                job.setPriority(int(key))
                print "urgentPrios - Set priority %s on job %d - %s" % (key, job.key(), job.name())
                job.commit()

                return True

    # Force a default priority of 50
    #if( job.priority() < 50 ):
    #    print "urgentPrios - Reset priority 50 on job %d - %s" % (job.key(), job.name())
    #    job.setPriority(50)
    #    job.commit()

    return True

VerifierPluginFactory().registerPlugin("urgentPrios", doThis)

