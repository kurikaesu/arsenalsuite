
from PyQt4.QtCore import *
from PyQt4.QtSql import *
from blur.Stone import *
from blur.Classes import *

from verifier_plugin_factory import *

def doThis(job):
    print "checkOnTens - checking %s" % job.name()

    if( job.jobType().name() == "3Delight" ):
        prevFrame = False
        numbers = job.jobTasks().frameNumbers()
        numbers.sort()
        if len(numbers) == 1:
            print "checkOnTens - single frame detected!"
            job.setPriority(18)
            job.setPacketType('continuous')
            job.commit()
            return True
        for frameNumber in numbers:
            if prevFrame and frameNumber - prevFrame > 9:
                print "checkOnTens - tens detected!"
                job.setPriority(29)
                job.setPacketType('continuous')
                job.commit()
                return True
            elif prevFrame and frameNumber - prevFrame > 4:
                print "checkOnTens - fives detected!"
                job.setPriority(30)
                job.setPacketType('continuous')
                job.commit()
                return True
            elif prevFrame:
                # only compare the first two frames..
                return True
            else:
                #print "checkOnTens - first frame is %i" % frameNumber
                prevFrame = frameNumber

    return True

VerifierPluginFactory().registerPlugin("checkOnTens", doThis)

