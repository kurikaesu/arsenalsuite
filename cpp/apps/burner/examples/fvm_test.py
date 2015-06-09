
from blur.quickinit import *
from blur.absubmit import *
import sys

testSequence = 'C:/temp/test_v2_0000.tif'
testOutput = 'C:/temp/test.avi'

job = JobFusionVideoMaker()
job.setJobType( JobType.recordByName( 'FusionVideoMaker' ) )
job.setSequenceFrameStart( 0 )
job.setSequenceFrameEnd( 400 )
job.setInputFramePath( testSequence )
job.setHost( Host.currentHost() )
job.setUser( User.currentUser() )
job.setName( 'Fusion_Video_Maker_Test' )
job.setOutputPath( testOutput )
job.setPriority( 50 )

# Create the submission object.  This is responsible for making sure the job has all prerequisites.
sub = Submitter()
# Will call qApp->exit() after the job submits or encounters a submission error
sub.setExitAppOnFinish( True )

sub.setJob( job )

# This adds a single output with a single task(1), it's not possible
# for multiple hosts to work on a single video output, so we do the whole thing as a single task
sub.addJobOutput( testOutput, "Output1", "1" )

# If this needs to wait for a render to finish, add the job as a dependency
# sub.addDependencies( JobList().append( job ) )

sub.submit()
sys.exit( QCoreApplication.instance().exec_() )

