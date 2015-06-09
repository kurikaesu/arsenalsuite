
from blur.Stone import *
from blur.Classes import *
from blur.Freezer import *
from PyQt4.QtCore import *
from PyQt4.QtSql import *
import traceback, sys, os

class CancelFrameInChainPlugin(MultiFrameViewerPlugin):
    def __init__(self):
        MultiFrameViewerPlugin.__init__(self)
        self.cancelledStatus = JobAssignmentStatus.recordByName("cancelled")

    def name(self):
        return QString("Cancel frame in chain")

    def icon(self):
        return QString("")

    def cancelIfNotDone(self, jobtask):
        if jobtask.status() != 'done':
            assignment = jobtask.jobTaskAssignment()
            assignment.setJobAssignmentStatus(self.cancelledStatus)
            assignment.commit()
            jobtask.setStatus('cancelled')
            jobtask.commit()

    def manipParentJobs(self, currentJob, wantedFrames):
        parentJobs = JobDep.recordsByJob(currentJob)
        for job in parentJobs:
            manipParentJobs(job, wantedFrames)

        tasks = currentJob.jobTasks().sorted('jobtask')
        for task in tasks:
            if task.frameNumber() in wantedFrames:
                self.cancelIfNotDone(task)

    def manipChildJobs(self, currentJob, wantedFrames):
        childJobs = JobDep.recordsByDep(currentJob)
        for job in childJobs:
            manipChildJObs(job, wantedFrames)

        tasks = currentJob.jobTasks().sorted('jobTask')
        for task in tasks:
            if task.frameNumber() in wantedFrames:
                self.cancelIfNotDone(task)

    def view(self, jobTaskList):
        for task in jobTaskList:
            self.cancelIfNotDone(task)

        self.manipParentJobs(task.job(), jobTaskList.frameNumbers())
        self.manipChildJobs(task.job(), jobTaskList.frameNumbers())

MultiFrameViewerFactory.registerPlugin( CancelFrameInChainPlugin() )
