
#include <qpushbutton.h>

#include "servicedialog.h"
#include "hostdialog.h"
#include "hostinterfacedialog.h"
#include "fieldlineedit.h"
#include "fieldtextedit.h"

HostDialog::HostDialog( QWidget * parent )
: QDialog( parent )
{
	setupUi( this );

	connect( mServiceButton, SIGNAL( clicked() ), SLOT( slotEditServices() ) );
	connect( mInterfacesButton, SIGNAL( clicked() ), SLOT( slotEditInterfaces() ) );

	mProxy = new RecordProxy( this );
	mNameEdit->setProxy( mProxy );
	mDescriptionText->setProxy( mProxy );
	mOSEdit->setProxy( mProxy );
	mSerialEdit->setProxy( mProxy );

	mProxy->setRecordList( mHost );
}

void HostDialog::slotEditServices()
{
	// Generate a primary key for this host
	host().key(true);
	ServiceDialog * sd = new ServiceDialog( this );
	sd->setHost( mHost );
	sd->exec();
	delete sd;
}

void HostDialog::slotEditInterfaces()
{
	// Generate a primary key for this host
	host().key(true);
	HostInterfaceDialog * hid = new HostInterfaceDialog( this );
	hid->setHost( mHost );
	hid->exec();
	delete hid;
}

void HostDialog::setHost( const Host & h )
{
	mOnlineCheck->setChecked( h.online() );
	mMemorySpin->setValue( h.memory() );
	mMhzSpin->setValue( h.mhz() );
	mCpusSpin->setValue( h.cpus() );
	mProxy->setRecordList( h, false, false );
	mAllowMappingCheck->setChecked( h.allowMapping() );
	mAllowSleepCheck->setChecked( h.allowSleep() );
	mHost = h;
}

Host HostDialog::host()
{
	mProxy->applyChanges( false/*commit*/ );

	// sync our host record with the modified one
	mHost = mProxy->records()[0];

	mHost.setOnline( mOnlineCheck->isChecked() ? 1 : 0 );
	mHost.setMemory( mMemorySpin->value() );
	mHost.setMhz( mMhzSpin->value() );
	mHost.setCpus( mCpusSpin->value() );
	mHost.setAllowMapping( mAllowMappingCheck->isChecked() );
	mHost.setAllowSleep( mAllowSleepCheck->isChecked() );
	return mHost;
}

void HostDialog::accept()
{
	// Set the changes
	host();
	mHost.commit();
	QDialog::accept();
}

void HostDialog::reject()
{
	//mProxy->rejectChanges(
	QDialog::reject();
}

