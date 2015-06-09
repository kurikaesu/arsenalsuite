from PyQt4.QtCore import *
from PyQt4.QtSql import *
from blur.Stone import *
from blur.Classes import *

from verifier_plugin_factory import *

def doThis(job):
    if job.checkFileMd5():
        filePath = getLocalFilePath(job)
        filePathExtra = re.sub('(\.zip|\.max)','.txt',filePath)

        if not QFile.exists( filePath ):
            print "Job file not found:", filePath
            return False

        if filePath in execReturnStdout( 'lsof %s 2>&1' % (filePath) ):
            print "Job file still open:", filePath
            return False

        if config.spoolDir != "":
            if not str(job.key()) in filePath:
                print "Job file without job key:", filePath
                return False

        md5sum = execReturnStdout('md5sum ' + filePath)[:32].lower()

        if md5sum != str(job.fileMd5sum()).lower():
            print "Job file %s md5sum doesnt match the database local md5sum: %s db md5sum: %s" % (filePath, md5sum, job.fileMd5sum())
            job.setStatus( 'corrupt' )
            cleanupJob(job)
            job.setCleaned(True)
            job.commit()
            host = job.host()
            notifylist = "it:e"
            notifySend( notifyList = notifylist, subject = 'Corrupt jobfile from host ' + host.name(),
                        body = "The following host just uploaded a corrupt job file to the Assburner spool.\n"
                            + "Host: %s\nJob: %s\nJobID: %i\n\n" % (host.name(), job.name(), job.key()) )
            return False

        job.setFileSize( QFileInfo( filePath ).size() )
    return True

VerifierPluginFactory().registerPlugin("check_md5", doThis)
