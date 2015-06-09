
import manager
from blur.quickinit import *

def dbe(sql,args = []):
	return Database.current().exec_(sql,args)

def clearTables(tables):
	for table in tables:
		dbe("DELETE FROM " + table)

def clearDatabase():
	clearTables( ['Job','JobStatus','JobTask','JobAssignment','JobTaskAssignment','JobService','Service','Host','HostService','License'] )

def createHost(name,services,initialStatus='ready'):
	host = Host().setName(name).setOnline(1).commit()
	for service in services:
		HostService().setHost(host).setService(service).setEnabled(True).commit()
	host.hostStatus().setSlaveStatus(initialStatus).commit()
	return host

def createService(name,license=License()):
	return Service().setService(name).setEnabled(True).setLicense(license).commit()

def createBatchJob(name,priority,services=[]):
	job = JobBatch().setName(name).setStatus('submit').setPriority(priority).setCmd('sleep 500').setJobType(JobType.recordByName('Batch')).commit()
	task = JobTask().setJob(job).setFrameNumber(1).setStatus('new').commit()
	for service in services:
		JobService().setJob(job).setService(service).commit()
	job.setStatus('verify').commit()
	job.setStatus('ready').commit()
	return job
	
# We should split up assburner so that we can have pyassburner and pass
# a host record to the slave, then we can test actual assburner code
def simulateAssburner():
	dbe("UPDATE HostStatus SET slavestatus='ready' WHERE slavestatus='starting'")

def testSingleAssignment():
	clearDatabase()
	
	# Setup Service with license
	assburner = createService('Assburner')
	service = createService('Test')
	host = createHost('Host',[assburner,service])
	job1 = createBatchJob('Test1',50,[assburner])
	manager.run_loop()
	
	# Job1 should get assigned to host
	assert( host.hostStatus().reload().slaveStatus() == 'ready' )
	assert( job1.jobTasks()[0].host() == host )
	assert( host.activeAssignments().size() == 1 )
	assert( host.activeAssignments()[0] == job1.jobTasks()[0].jobTaskAssignment().jobAssignment() )

def testDoubleAssignment():
	clearDatabase()
	
	# Setup Service with license
	assburner = createService('Assburner')
	service = createService('Test')
	host = createHost('Host',[assburner,service])
	job1 = createBatchJob('Test1',49,[assburner])
	job2 = createBatchJob('Test2',50,[assburner])
	
	host.hostStatus().setAvailableMemory( 50 ).commit()
	
	manager.run_loop()
	manager.run_loop()
	
	# Job1 should be assigned
	assert( job1.jobTasks()[0].host() == host )
	
	# Job2 should not be assigned because it does not have avg memory stats yet
	assert( host.activeAssignments().size() == 1 )
	
	job2.jobStatus().setAverageMemory( 55 ).commit()
	manager.run_loop()
	
	# Job2 should not be assigned because host does not have enough memory for both jobs
	assert( host.activeAssignments().size() == 1 )
	
	job2.jobStatus().setAverageMemory( 20 ).commit()
	manager.run_loop()
	
	# Job1 should get assigned to host now that there is enough memory for both
	assert( host.hostStatus().reload().slaveStatus() == 'ready' )
	assert( job2.jobTasks()[0].host() == host )

# Unassigns 'assigned' tasks from lower priority job to make available for higher priority job
def testUnassignment():
	clearDatabase()
	
	# Setup Service with license
	assburner = createService('Assburner')
	service = createService('Test')
	host = createHost('Host',[assburner,service])
	job1 = createBatchJob('Test1',50,[assburner])
	manager.run_loop()
	
	# Job1 should get assigned to host
	assert( host.hostStatus().reload().slaveStatus() == 'ready' )
	print( job1.jobTasks()[0].dump() )
	assert( job1.jobTasks()[0].host() == host )
	
	job2 = createBatchJob('Test2',1,[service])
	manager.run_loop()
	simulateAssburner()
	
	assert( job1.jobTasks()[0].status() == 'new' )
	manager.run_loop()
	assert( job2.jobTasks()[0].jobTaskAssignment().host() == host )

# Test that we don't unassign for a job that has no licenses available
def testLicenseUnassignment():
	print "\n\nTesting License Unassignment\n\n"
	clearDatabase()
	
	# Setup Service with license
	lic = License().setTotal(0).commit()
	assburner = createService('Assburner')
	service = createService('Test',lic)
	host = createHost('Host',[service,assburner])
	job1 = createBatchJob('Test1',50)
	manager.run_loop()
	
	# Job1 should get assigned to host
	assert( host.hostStatus().reload().slaveStatus() == 'assigned' )
	assert( job1.jobTasks()[0].host() == host )
	
	# Create higher priority job that current has no licenses available
	job2 = createBatchJob('Test2',1,[service])
	# Make sure nothing gets unassigned since it has not licenses
	manager.run_loop()
	assert( job1.jobTasks()[0].host() == host )
	
	# Give it a license and test that job Test1 gets unassigned
	lic.setTotal(1).commit()
	manager.run_loop()
	simulateAssburner()
	assert( job1.jobTasks()[0].status() == 'new' )
	manager.run_loop()
	assert( job2.jobTasks()[0].host() == host )
	
	# Create new high priority job with same license needs
	job3 = createBatchJob('Test3',1,[service])
	# Change current assigned jobs priority
	job2.setPriority(40).commit()
	manager.run_loop()
	simulateAssburner()
	Database.current().setEchoMode(Database.EchoSelect)
	print lic.reload().dump()
	Database.current().setEchoMode(0)
	assert( job2.jobTasks()[0].status() == 'new' )
	manager.run_loop()
	assert( job3.jobTasks()[0].host() == host )
	
if __name__=='__main__':
	manager.VERBOSE_DEBUG=True
	# Disable throttling
	manager.throttler.assignRate=9999999
	manager.throttler.assignLeft=9999999
	testSingleAssignment()
	testDoubleAssignment()
	#testUnassignment()
	#testLicenseUnassignment()
