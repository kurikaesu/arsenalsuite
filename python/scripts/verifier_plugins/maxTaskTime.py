
from PyQt4.QtCore import *
from PyQt4.QtSql import *
from blur.Stone import *
from blur.Classes import *

from verifier_plugin_factory import *

def doThis(job):
    print "maxTaskTime - checking %s" % job.name()

    job.setMaxTaskTime(0)

    if( job.jobType().name().contains("Mantra") ):
        job.setMaxQuietTime(1 * 60 * 60)
        job.setPacketType('iterative')
        job.setAutoAdaptSlots(0)
        job.setAssignmentSlots(6)
    if( job.jobType().name() == "3Delight" ):
        job.setPacketSize(1)
        job.setAutoAdaptSlots(0)
        job.setAssignmentSlots(6)
        job.setMinMemory(1000000*job.assignmentSlots())
        job.setMaxMemory(2000000*job.assignmentSlots())
        if not job.priority() == 29:
            job.setPacketType('iterative')

    if( job.jobType().name().contains( "Nuke" ) ):
        job.setAssignmentSlots(2)
        if( job.user().name().contains("maximilian.macewan") and (job.project().name() == "hf2-light" or job.project().name() == "hf2-comp") and job.name().contains("SkyLibrary: Create Proxy")):
            job.setMinMemory(9000000)
            job.setMaxMemory(16000000)
        else:
            job.setMinMemory(6000000)
            job.setMaxMemory(8000000)
    if( job.jobType().name().contains( "Naiad" ) ):
        job.setMinMemory(20000000)
        job.setMaxMemory(24000000)
    if( job.jobType().name().contains( "Batch" ) ):
        job.setAutoAdaptSlots(0)
        if (job.name().contains("_rib")):
            job.setMinMemory(4000000)
            job.setMaxMemory(8000000)
            job.setAssignmentSlots(1)
            job.setPacketSize(20)

    job.commit()
    return True

VerifierPluginFactory().registerPlugin("maxTaskTime", doThis)

