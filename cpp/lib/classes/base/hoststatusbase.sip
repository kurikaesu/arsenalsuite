
//
// Sets host.fkeyjob to 0, host.slaveFrames to ""
// Sets all jobtasks to new where fkeyhost= and status 'busy' or 'assigned'
// commits this host and the jobtasks
void returnSlaveFrames( bool commit=false );
