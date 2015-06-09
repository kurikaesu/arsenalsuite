
/* $Author$
 * $LastChangedDate: 2010-03-26 13:17:37 +1100 (Fri, 26 Mar 2010) $
 * $Rev: 9598 $
 * $HeadURL: svn://svn.blur.com/blur/branches/concurrent_burn/cpp/lib/assfreezer/src/batchsubmitdialog.cpp $
 */

#include <qmessagebox.h>
#include <qfiledialog.h>
#include <qlistwidget.h>
#include <qlineedit.h>
#include <qprocess.h>

#include "blurqt.h"
#include "md5.h"
#include "path.h"

#include "jobbatch.h"
#include "jobcannedbatch.h"
#include "jobtask.h"
#include "jobtype.h"
#include "user.h"

#include "submitter.h"

#include "batchsubmitdialog.h"
#include "hostselector.h"

BatchSubmitDialog::BatchSubmitDialog( QWidget * parent )
: QDialog( parent )
, mSaveCannedBatchMode( false )
{
	setupUi( this );
	connect( mAddButton, SIGNAL( clicked() ), SLOT( addFiles() ) );
	connect( mRemoveButton, SIGNAL( clicked() ), SLOT( removeFiles() ) );
	connect( mClearButton, SIGNAL( clicked() ), SLOT( clearFiles() ) );
	mProjectCombo->setSpecialItemText( "None" );
	mProjectCombo->setShowSpecialItem( true );
	mProjectCombo->setStatusFilters( ProjectStatus::recordByName( "Production" ) );
	// Default to priority 5
	mPrioritySpin->setValue( 5 );
	//mRunAsUserGroup->hide();
}

void BatchSubmitDialog::setSaveCannedBatchMode( bool scbm )
{
	mSaveCannedBatchMode = scbm;
	mFilesGroup->setVisible( !scbm );
	mCannedBatchGroup->setCheckable( !scbm );
	mSubmitButton->setText( scbm ? "Save Canned Batch Job" : "Submit" );
	mProjectLabel->setVisible( !scbm );
	mProjectCombo->setVisible( !scbm );
}

BatchSubmitDialog::~BatchSubmitDialog()
{}

void BatchSubmitDialog::setHostList( const HostList & hl )
{
	mHosts = hl;
}

void BatchSubmitDialog::setName( const QString & name )
{
	mJobNameEdit->setText( name );
}

void BatchSubmitDialog::setCommand( const QString & cmd )
{
	mCommandEdit->setText( cmd );
}

void BatchSubmitDialog::setDisableWow64FsRedirect( bool dfsr )
{
	mDisableWow64FsRedirectionCheck->setChecked( dfsr );
}

void BatchSubmitDialog::setCannedBatchGroup( const QString & grp )
{
	mCannedBatchGroupEdit->setText( grp );
}

/*
void BatchSubmitDialog::slotEditHostList()
{
	HostSelector * hs = new HostSelector( this );
	hs->setHostList( mHosts );
	if( hs->exec() == QDialog::Accepted )
		mHosts = hs->hostList();
	delete hs;
}
*/

void BatchSubmitDialog::addFiles()
{
	QStringList files = QFileDialog::getOpenFileNames( this, "Select Batch Job Files" );
	mFilesList->addItems( files );
}

void BatchSubmitDialog::removeFiles()
{
	QList<QListWidgetItem*> sel = mFilesList->selectedItems();
	foreach( QListWidgetItem * item, sel )
		delete item;
}

void BatchSubmitDialog::clearFiles()
{
	mFilesList->clear();
}

void BatchSubmitDialog::accept()
{
	QString archivePath;
	QString jobName = mJobNameEdit->text();
	if( jobName.isEmpty() ) {
		QMessageBox::critical( this, "Job name missing", "Job must have a name" );
		return;
	}

	if( mSaveCannedBatchMode || mCannedBatchGroup->isChecked() ) {
		QString grp = mCannedBatchGroupEdit->text();
		if( grp.isEmpty() ) {
			QMessageBox::critical( this, "Canned Batch Group is Empty", "Canned Batch Jobs require a group" );
			return;
		}
		JobCannedBatch jcb = JobCannedBatch::recordByGroupAndName( grp, jobName );
		if( jcb.isRecord() ) {
			if( QMessageBox::warning( this, "Overwrite Canned Batch Job?", 
				"A Canned Batch Job with this Group/Name already exists, would you like to overwrite it?",
				QMessageBox::Ok | QMessageBox::Cancel ) 
				!= QMessageBox::Ok ) {
				return;
			}
		}
		jcb.setName( jobName );
		jcb.setGroup( grp );
		jcb.setCmd( mCommandEdit->text() );
		jcb.setDisableWow64FsRedirect( mDisableWow64FsRedirectionCheck->isChecked() );
		jcb.commit();
	}

	if( mSaveCannedBatchMode ) {
		QDialog::accept();
		return;
	}

	if( mFilesList->count() > 0 ) {
		/*
		 * Create the temp directory and copy all files there
		 */
		Path path( QDir::tempPath() + QDir::separator() + "batch_temp" + QDir::separator() );

		if( !path.mkdir() ) {
			LOG_1( "Couldn't create temp directory: " + path.path() );
			reject();
			return;
		}
		
		bool error = false;
		int end = mFilesList->count();
		for( int i = 0; i < end; i++ ) {
			QListWidgetItem * item = mFilesList->item( i );
			if( !Path::copy( item->text(), path.path() ) ) {
				LOG_1( "Failed to copy " + item->text() + " to " + path.path() );
				error = true;
				break;
			}
		}
		
		/*
		 * Remove the temp directory if we had any problems
		 */
		if( error ) {
			path.remove();
			reject();
			return;
		}
		
		/*
		 * Create the archive
		 */
		archivePath = QDir::tempPath() + QDir::separator() + "batcharchive.zip";
		QProcess p;
#ifdef Q_OS_WIN
		p.start( "c:/blur/zip.exe -j " + archivePath + " " + path.path() );
#else
		p.start( "zip -jr " + archivePath + " " + path.path() );
#endif
		p.waitForFinished();
		if( p.exitCode() != 0 ) {
			LOG_1( "Error creating archive, return value was " + QString::number( p.exitCode() ) + ", output was: " + QString::fromLatin1( p.readAllStandardOutput() ) );
			error = true;
		}
		
		// Remove dir
		path.remove();
		
		if( error ) {
			reject();
			return;
		}
	}
	
	/*
	 * Submit the job
	 */
	JobBatch jb;
	jb.setJobType( JobType::recordByName( "Batch" ) );
	jb.setName( jobName );
	jb.setPacketSize( 1 );
	jb.setPacketType( "preassigned" );
	jb.setHost( Host::currentHost() );
	jb.setPriority( mPrioritySpin->value() );
	jb.setCmd( mCommandEdit->text() );
	jb.setDisableWow64FsRedirect( mDisableWow64FsRedirectionCheck->isChecked() );
	jb.setUser( User::currentUser() );
	jb.setFileName( archivePath );
	jb.setProject( mProjectCombo->project() );
	if( mMaxHostsSpin->value() > 0 )
		jb.setMaxHosts( mMaxHostsSpin->value() );
	if( mRunAsUserGroup->isChecked() ) {
		jb.setUserName( mUserName->text() );
		jb.setDomain( mDomain->text() );
		jb.setPassword( mPassword->text() );
	}

	Submitter * s = new Submitter();
	// Commits the job, status set to 'submit'
	s->setJob( jb );
	
	int i=0;
	JobTaskList tasks;
	foreach( Host h, mHosts ) {
		JobTask jt;
		jt.setFrameNumber( i++ );
		jt.setHost( h );
		tasks += jt;
	}
	tasks.setJobs( jb );
	tasks.setStatuses( "new" );
	tasks.commit();

	s->addTasks( tasks );
	s->addServices( ServiceList() += Service::recordByName( "Assburner" ) );
	s->setSubmitSuspended( mStatusSuspendedRadio->isChecked() );
	s->submit();

	QDialog::accept();
}

