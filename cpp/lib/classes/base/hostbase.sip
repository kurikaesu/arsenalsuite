
JobAssignmentList activeAssignments() const;
static JobAssignmentList activeAssignments(HostList hosts);

static MappedHost currentHost();
static QString currentHostName();
int syslogStatus();
static MappedHost autoRegister();
void updateHardwareInfo();
