

#ifndef OOP_BURNER_H
#define OOP_BURNER_H

#include "jobburner.h"

class Win32Process;

// Out of process Burner
class OOPBurner : public JobBurner
{
public:
	OOPBurner( const JobAssignment & jobAssignment, Slave * slave );
	virtual ~OOPBurner();

	virtual QStringList processNames() const;

	virtual QString executable();

	virtual QString workingDirectory();

	virtual QStringList buildCmdArgs();

	virtual QStringList environment();

	virtual void start();

	virtual void startBurn();
	
	virtual void cleanup();

protected:
	virtual void startProcess();

	virtual bool checkup();

	virtual void slotProcessExited();
	virtual void slotProcessStarted();

	virtual void slotProcessError( QProcess::ProcessError );

	Win32Process * mProcess;
};

#endif // OOP_BURNER_H
