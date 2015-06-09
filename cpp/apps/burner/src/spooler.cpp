
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of Arsenal.
 *
 * Arsenal is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Arsenal is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Arsenal; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id$
 */

#include <qdir.h>
#include <qfileinfo.h>
#include <qftp.h>
#include <qregexp.h>
#include <qtimer.h>
#include <qthread.h>
#include <qprocess.h>

#include <math.h>

#ifdef Q_OS_LINUX
#include <sys/vfs.h>
#endif

#include "abdownloadstat.h"
#include "common.h"
#include "config.h"
#include "job.h"
#include "jobtype.h"
#include "path.h"
#include "maindialog.h"
#include "process.h"
#include "syslog.h"
#include "process.h"
#include "service.h"
#include "blurqt.h"
#include "md5.h"

#include "spooler.h"


SpoolItem::SpoolItem( Spooler * spooler, const Job & job )
: mState( Idle )
, mRefCount( 0 )
, mJob( job )
, mSpooler( spooler )
, mDownload( 0 )
, mUnzipProcess( 0 )
, mCheckupTimer( 0 )
, mUnzipFailureCount( 0 )
, mUnzipTimeout( 300000 )
, mCopied( false )
, mUnzipped( false )
{
	mSrcFile = job.getValue( "fileName" ).toString();
	QFileInfo srcInfo( mSrcFile );
	QFileInfo destInfo( mSpooler->spoolDir() + "/" + srcInfo.fileName() );
	mDestFile = destInfo.filePath();
}

SpoolItem::~SpoolItem()
{
	LOG_5("SpoolItem::dtor called");
	cleanupZipFolder();
	delete mUnzipProcess;
	delete mCheckupTimer;
	delete mDownload;
	LOG_5("SpoolItem::dtor done");
}

bool SpoolItem::isZip()
{
	return QFileInfo( mDestFile ).suffix() == "zip";
}

bool SpoolItem::isUnzipped()
{
	return mUnzipped;
}

void SpoolItem::unzip()
{
	if( !mUnzipped && isZip() ) {
		checkUnzip();
	}
}

void SpoolItem::cleanupZipFolder()
{
	LOG_5("SpoolItem::cleanupZipFolder() called");
	if( mUnzipped ) {
		QString err;
		if( !Path::remove( zipFolder(), true, &err ) ) {
			_error( "Failed to remove zip folder completely: " + zipFolder() + " error was: " + err );
		}
		mUnzipped = false;
	}
	LOG_5("SpoolItem::cleanupZipFolder() done");
}

QString SpoolItem::zipFolder()
{
	return mSpooler->spoolDir() + "/" + QString::number( mJob.key() ) + "/";
}

void SpoolItem::_error( const QString & msg )
{
	int oldState = mState;
	_changeState( Errored );
	emit errored( msg, oldState );
}

void SpoolItem::_changeState( State state )
{
	int oldState = mState;
	mState = state;
	emit stateChanged( oldState, state );
}

void SpoolItem::_finished()
{
	emit completed();
	_changeState( Idle );
}

void SpoolItem::startCopy()
{
	LOG_5( "SpoolItem::startCopy" );

	// Already copying or unzipping	
	if( mState != Idle ) return;

	// Already copied, check unzip value
	if( mCopied ) {
		checkUnzip();
		return;
	}
	
	// No file for this job
	if( mJob.getValue("fileName").toString().isEmpty() ) {
		_finished();
		return;
	}


	QFileInfo destInfo( mDestFile );
	// Remove existing file if it is incorrect size
	if( destInfo.exists() && destInfo.size() != mJob.fileSize() ) {
		LOG_3( "SpoolItem: Existing file " + destInfo.filePath() + " doesn't match filesize in database, deleting." );
		if( !QFile::remove( destInfo.filePath() ) ) {
			_error( "Couldn't delete existing invalid jobfile: " + destInfo.filePath() );
			return;
		}
		destInfo = QFileInfo( mDestFile );
	}

	// File already here and valid, check unzip
	if( destInfo.exists() ) {
		LOG_3( "SpoolItem::startCopy: file already here" );
		mCopied = true;
		checkUnzip();
		return;
	}

	// We currently require 250 megs of free space on top of the size required for
	// the job file. TODO This should be configurable per jobtype(per job?), and should
	// also take into account the size need once the archive is unzipped when the
	// file is an archive.
	int fsRequired = 1024*1024*250; // 250 megs
	if( !destInfo.exists() )
		fsRequired += mJob.fileSize();

	// Ensure enough space
	if( !mSpooler->ensureDriveSpace( fsRequired ) ) {
		_error( "Insufficient drive space for job" );
		host_error( "Insufficient drive space for job", "startCopy", mJob.jobType().service() );
		return;
	}

	mDownload = new MultiDownload( this );
	connect( mDownload, SIGNAL( progress( qint64, qint64 ) ), SLOT( downloadProgress( qint64, qint64 ) ) );
	connect( mDownload, SIGNAL( stateChanged( int ) ), SLOT( downloadStateChanged( int ) ) );
	connect( mDownload, SIGNAL( torrentHandleChanged( const libtorrent::torrent_handle & ) ), SLOT( setTorrentHandle( const libtorrent::torrent_handle & ) ) );

	mDownload->setup( mSrcFile, mDestFile );
	mDownload->start();
	_changeState( Copy );
}

void SpoolItem::cancelCopy()
{
	LOG_3( "SpoolItem::cancelCopy" );
	if( mUnzipProcess ) {
		mUnzipProcess->disconnect( this );
		mUnzipProcess->kill();
		mUnzipProcess->deleteLater();
		mUnzipProcess = 0;
	}
	if( mCheckupTimer ) {
		mCheckupTimer->disconnect( this );
		mCheckupTimer->deleteLater();
		mCheckupTimer = 0;
	}
	if( mDownload ) {
		mDownload->disconnect( this );
		mDownload->cancel();
		mDownload->deleteLater();
		mDownload = 0;
	}

	mSpooler->cancelCopy( this );

	QFile::remove( mDestFile );
	cleanupZipFolder();
	LOG_3( "SpoolItem::cancelCopy done" );
}

bool SpoolItem::checkMd5()
{
	// if md5sum in DB, create hash of local jobfile, compare with DB
	QString dbMd5sum = mJob.fileMd5sum();
	LOG_3("SpoolItem::checkUnzip() mJob has fileMd5sum in DB of " + dbMd5sum );
	if ( !dbMd5sum.isEmpty() ) {
		Md5 md5;
		QString localMd5sum;
		localMd5sum = md5.calcMd5( mDestFile );
		LOG_3("SpoolItem::checkUnzip() Md5::calcMd5 of " + mDestFile + " gave md5sum of:" + localMd5sum );
		if (localMd5sum != dbMd5sum) { // hashes don't match, error out
			_error( QString("Jobfile corruption detected! Job %1, local file %2. Database MD5 hash is %3, local hash is %4")
			       .arg( mJob.key() )
				   .arg( mDestFile )
				   .arg( dbMd5sum )
				   .arg( localMd5sum ) );
			return false;
		}
	}
	return true;

}

bool SpoolItem::checkUnzip()
{
	if( !Path::checkFileFree( mDestFile ) ) {
		_error( "SpoolItem::checkUnzip: Couldn't open the file" );
		return false;
	}

	if( !checkMd5() )
		return false;

	if( isZip() && !isUnzipped() ) {
		_changeState( Unzip );
		QString dest( zipFolder() );
		QString unzip( "c:\\blur\\assburner\\unzip.exe" ); // TODO, make this come from CWD, or from .ini 
		
		if( !QFile::exists( unzip ) ) {
			_error( "SpoolItem::checkUnzip: unzip.exe is missing at: " + unzip );
			host_error( "Spoolitem::checkUnzip: unzip.exe is missing at: " + unzip, "SpoolItem::checkUnzip", mJob.jobType().service() );
			return false;
		}

		QString cmd = unzip + " " + mDestFile + " -d " + dest;
		LOG_3( "SpoolItem::checkUnzip: Running unzip cmd: " + cmd );
		
		QString error;
		if( !Path::remove( dest, true, &error ) ) {
			error = "SpoolItem::checkUnzip: couldn't remove existing render directory at: " + dest + error;
			_error( error );
			host_error( error, "checkUnzip", mJob.jobType().service() );
			return false;
		}

		mUnzipProcess = new QProcess();
		connect( mUnzipProcess, SIGNAL( readyReadStandardOutput() ), SLOT( unzipReadyRead() ) );
		connect( mUnzipProcess, SIGNAL( finished( int ) ), SLOT( unzipExited() ) );
		connect( mUnzipProcess, SIGNAL( error( QProcess::ProcessError ) ),SLOT( unzipExited() ) );
		
		mUnzipProcess->start(cmd);
	
		// Give the unzip 5 minutes to complete, then kill it
		mCheckupTimer = new QTimer( this );
		connect( mCheckupTimer, SIGNAL( timout() ), SLOT( unzipTimedOut() ) );
		mCheckupTimer->start( mUnzipTimeout );
		return true;
	}

	_finished();
	return true;
}

void SpoolItem::unzipExited()
{
	if( mUnzipProcess->exitCode() != 0 )
	{
		_error( "SpoolItem::unzipExited: Error unzipping archive" );
		return;
	}

	mUnzipProcess->disconnect( this );
	mUnzipProcess->deleteLater();
	mUnzipProcess = 0;

	mCheckupTimer->disconnect( this );
	mCheckupTimer->deleteLater();
	mCheckupTimer = 0;

	mUnzipped = true;
	_finished();
}

void SpoolItem::unzipReadyRead()
{
	if( !mUnzipProcess ) {
		LOG_1( "SpoolItem::unzipReadyRead called while mUnzipProcess=0" );
		return;
	}
	mUnzipProcess->setReadChannel( QProcess::StandardOutput );
	while( mUnzipProcess->canReadLine() ) {
		LOG_5( "SpoolItem::unzipReadyRead: Unzip stdout: " + QString::fromAscii(mUnzipProcess->readLine()) );
	}
}

void SpoolItem::unzipTimedOut()
{
	if( mUnzipProcess ) {
		LOG_1( "SpoolItem::unzipTimedOut: Unzip Timed Out" );
		mUnzipProcess->disconnect( this );
		mUnzipProcess->kill();
		mUnzipProcess->deleteLater();
		mUnzipProcess = 0;

		mCheckupTimer->disconnect( this );
		mCheckupTimer->deleteLater();
		mCheckupTimer = 0;

		mUnzipFailureCount++;

		if( mUnzipFailureCount > 1 ) { // We'll try twice
			_error( "The Unzip Timed Out After " + QString::number( mUnzipTimeout ) + " seconds" );
			return;
		}

		// Try again, double the timeout time
		mUnzipTimeout *= 2;
		checkUnzip();
	} else {
		LOG_1( "SpoolItem::unzipTimedOut called when mUnzipProcess is 0" );
	}
}

void SpoolItem::downloadProgress( qint64 done, qint64 total )
{
	LOG_5( "SpoolItem::downloadProgress: " + QString::number( done / 1024 ) + " of " + QString::number( total / 1024 ) + " kbytes downloaded" );
	emit copyProgressChange( (100*done) / total );
}

void SpoolItem::downloadStateChanged( int state )
{
	if( state == AbstractDownload::Error ) {
		_error( mDownload->errorMsg() );
		return;
	} else if( state == AbstractDownload::Finished ) {
		int mseconds = qMax( mDownload->elapsed(), 1 );
		int wanted = mDownload->size();
	
		LOG_3( QString("SpoolItem::downloadStateChanged: ") + mDownload->typeName() + " finished " + mJob.name()
				+ " size " + QString::number( wanted )
				+ QString(" in %1:%2:%3").arg( mseconds / 3600000 ).arg( (mseconds / 60000) % 60 ).arg( (mseconds/1000) % 60 )
				+ " at " + add_suffix( wanted / float(mseconds/1000.0) ).c_str() + "/S" );
	
		mSpooler->addDownloadStat( mDownload->typeName(), mJob, wanted, mseconds/1000 );

		emit copyProgressChange( 100 );

		mDownload->disconnect( this );
		mDownload->deleteLater();
		mDownload = 0;

		checkUnzip();
		return;
	}
}

void SpoolItem::ref()
{
	mRefCount++;
}

void SpoolItem::deref()
{
	mRefCount--;
	if( mRefCount == 0 )
		cancelCopy();
}

SpoolRef::SpoolRef( SpoolItem * item )
: mSpoolItem( item )
{
	item->ref();
	connect( item, SIGNAL( copyProgressChange( int ) ), SIGNAL( copyProgressChange( int ) ) );
	connect( item, SIGNAL( errored( const QString &, int ) ), SIGNAL( errored( const QString &, int ) ) );
	connect( item, SIGNAL( stateChanged( int, int ) ), SIGNAL( stateChanged( int, int ) ) );
	connect( item, SIGNAL( completed() ), SIGNAL( completed() ) );
}

void SpoolRef::start()
{
	mSpoolItem->startCopy();
}

SpoolRef::~SpoolRef()
{
	mSpoolItem->deref();
}

Spooler::Spooler( QObject * parent )
: QObject( parent )
, mRecordDownloadStats( false )
, mCheckTimer( 0 )
{
	IniConfig & c = config();
	c.pushSection( "Assburner" );
	mSpoolDir = c.readString( "SpoolDirectory", "c:/blur/assburner/spool" );
	c.popSection();
	mRecordDownloadStats = Config::getBool( "assburnerRecordDownloadStats", false );

	if( !QDir( mSpoolDir ).exists() ){
		LOG_5( "Creating Spool Dir" );
		QDir().mkdir( mSpoolDir );
	}

	// Cleanup all the leftover files right when we start
	fileCleanup();

	// Check the spool items every 30 seconds to make sure they 
	// are valid
	mCheckTimer = new QTimer( this );
	connect( mCheckTimer, SIGNAL( timeout() ), SLOT( cleanup() ) );

	// Every 5 minutes, check for cleanable jobs
	mCheckTimer->start( 5 * 60 *1000 );
}

Spooler::~Spooler()
{
	for( SpoolIter it = mSpool.begin(); it != mSpool.end(); ++it )
	{
		SpoolItem * si = *it;
		emit cancelled( si );
		si->deleteLater();
	}
	mSpool.clear();

}

SpoolItem * Spooler::startCopy( const Job & job )
{
	SpoolItem * si = itemFromJob( job );
	if( !si ) {
		si = new SpoolItem( this, job );
		si->ref();
		mSpool += si;
	}
	return si;
}

void Spooler::cancelCopy( SpoolItem * item )
{
	if( mSpool.contains( item ) ) {
		mSpool.removeAll( item );
		emit cancelled( item );
		item->deleteLater();
	} else {
		LOG_1( "Spooler::cancelCopy: ERROR: item not found in spool" );
	}
}

QString Spooler::spoolDir() const
{
	return mSpoolDir;
}

void Spooler::cleanup()
{
	SpoolList spoolCopy(mSpool);

	for( SpoolIter it = spoolCopy.begin(); it != spoolCopy.end(); ++it )
	{
		SpoolItem * si = *it;
		Job j = si->mJob;
		LOG_3( "Spooler::cleanup: Checking Job: " + j.name() );
		j.reload();
		if( !j.isRecord() || j.status() == "done" || j.status() == "deleted" || j.status() == "suspended" ) {
			LOG_3( "Spooler::cleanup: Job has status " + (j.isRecord() ? j.status() : QString("deleted")) + ", Deleting Spool Item" );
			si->deref();
		}
	}
}

bool Spooler::checkDriveSpace( qint64 required )
{
#ifdef Q_OS_WIN
	qint64 freeBytes;
	BOOL ret = GetDiskFreeSpaceEx( L"C:\\\\", (PULARGE_INTEGER)&freeBytes, 0, 0 );
	if( !ret ) {
		LOG_1( "JB: GetDiskFreeSpaceEx call failed with error: " + QString::number( GetLastError() ) );
		return false;
	}
	LOG_1( QString( "JB: Free Space %1, Needed %2" ).arg( freeBytes ).arg( required ) );
	return (ret && freeBytes > required );
#endif // Q_OS_WIN
#ifdef Q_OS_LINUX
	struct statfs stats;
	unsigned long long available;

	QString path = mSpoolDir + "/.";
	statfs(path.toLatin1().constData(), &stats);
	available = ((unsigned long long)stats.f_bavail) *
              ((unsigned long long)stats.f_bsize);
	LOG_1( QString( "JB: Free Space %1, Needed %2" ).arg( available ).arg( required ) );
	return (available > required);
#endif // Q_OS_LINUX
	return false;
}

bool Spooler::ensureDriveSpace( qint64 required )
{
	if( !checkDriveSpace( required ) ) {
		fileCleanup();
		return checkDriveSpace( required );
	}
	return true;
}

void Spooler::deleteFiles()
{
	for(QStringList::Iterator it = mFilesToDelete.begin(); it != mFilesToDelete.end(); ) {
		QFileInfo destInfo( *it );
		QFile destFile( *it );
		if( destFile.exists() ) {
			LOG_5( "Spooler::deleteFiles attempting to delete done jobfile " + destInfo.filePath() );
			if (! destFile.remove()) {
				LOG_5( "Spooler::deleteFiles QFile::remove failed -- " + destFile.errorString() );
				++it;
			} else {
				LOG_5("Spooler::deleteFiles deleted jobfile -- " + destInfo.filePath() ); 
				it = mFilesToDelete.erase( it );
			}
		} else {
			LOG_5("Spooler::deleteFiles done jobfile " + destInfo.filePath() + " already deleted. removing from to-delete list");
			it = mFilesToDelete.erase( it );
		}

		// attempt to remove max-created temp XML file
		LOG_5( "Spooler::deleteFiles starting regex search for maxhold jobId from " + destInfo.filePath() );
		QRegExp rx( "maxhold(\\d+)" ); //TODO also machine archiveXXXX

		if (!rx.isValid())
			LOG_5( "Spooler::deleteFiles QRegExp not valid -- " + rx.errorString() );

		rx.setCaseSensitivity( Qt::CaseInsensitive );
		if ( destInfo.path().contains( rx ) )  { // found jobId
			QString jobId = rx.cap( 1 );
			QDir dir( mSpoolDir );
			QFileInfoList fil = dir.entryInfoList( QStringList() << ("maxhold" + jobId + "*.xml"), QDir::Files );
			
			for( int i=0; i<fil.size(); i++ ) {
				QFileInfo fi( fil.at(i) );
				QString tempFileStr = fi.filePath();
				LOG_5("Spooler::deleteFiles attempting to remove done tempfile " + tempFileStr );
				QFile tempFile(tempFileStr);
				if( tempFile.remove() ) {
					LOG_5( "Spooler::deleteFiles delete tempfile: " + tempFileStr );
				} else {
					LOG_5( "Spooler::deleteFiles QFile::remove failed -- " + tempFile.errorString() );
				}
			}

		}
	}
}

int Spooler::fileCleanup()
{
	QDir dir( mSpoolDir );
	QFileInfoList fil = dir.entryInfoList( QDir::Files );
	int spaceSaved = 0;

	for( int i=0; i<fil.size(); i++ ) {
		QFileInfo fi( fil.at(i) );
		QString ext = fi.suffix();
		
		// Currently we'll just clean these file types
		if( ext != "max" && ext != "zip" && ext != "xml" && ext != "mx" )
			continue;

		// Find the job id. ex job_is_name_bla_blaXXXXXX.max
		int id_start, id_end;
		QString name = fi.fileName();

		//  First find the last X
		for( id_end = name.length()-1; id_end >= 0 && !name[id_end].isNumber(); id_end-- );

		// No digits in the filename, well, delete the fucker anyway
		if( id_end >= 0 ) {
			// Then find the first X
			for( id_start = id_end; id_start > 0 && name[id_start-1].isNumber(); id_start-- );
			
			int id = name.mid( id_start, id_end-id_start ).toInt();
			
			if( id > 0 ) {
				Job j( id );
				if( j.isRecord() && j.status() != "done" && j.status() != "deleted" )
					continue;
			}
		}

		int space = fi.size();
		if( QFile::remove( fi.filePath() ) ) {
			LOG_5( "Spooler::fileCleanup - Removed File: " + name );
			spaceSaved += space;
		}
	}

	fil = dir.entryInfoList( QDir::Dirs );
	for( int i=0; i<fil.size(); i++ ) {
		QFileInfo fi( fil.at(i) );
		if( QRegExp("^[0-9]+$" ).exactMatch(fi.fileName()) ) {
			Job j( fi.fileName().toInt() );
			if( !j.isRecord() || !itemFromJob(j) ) {
				LOG_5( "Spooler::fileCleanup: Deleting Spool Directory: " + fi.fileName() );
				QString error;
				long long size = Path::dirSize( fi.filePath() );
				if( !Path::remove( fi.filePath(), true, &error ) )
					LOG_5( "Spooler::fileCleanup: Failed to remove directory, reason was: " + error );
				else
					spaceSaved += size;
			}
		}
	}
	LOG_5( "Spooler::fileCleanup: Finish, Freed " + QString::number( spaceSaved ) + " bytes" );
	return spaceSaved;
}

void Spooler::addDownloadStat( const QString & type, const Job & job, int size, int time )
{
	if( mRecordDownloadStats ) {
		AbDownloadStat stat;
		stat.setHost( Host::currentHost() );
		stat.setType( type );
		stat.setSize( size );
		stat.setTime( time );
		stat.setColumnLiteral( "finished", "NOW()" );
		stat.setAbrev( QString("$Date$").remove(QRegExp("[^\\d]")).toInt() );
		stat.setJob( job );
		stat.commit();
	}
}

SpoolItem * Spooler::itemFromJob( const Job & job )
{
	for( SpoolIter it = mSpool.begin(); it != mSpool.end(); ++it )
	{
		if( (*it)->mJob == job )
			return *it;
	}
	return 0;
}

std::string to_string(float v, int width, int precision = 3)
{
	std::stringstream s;
	s.precision(precision);
	s.flags(std::ios_base::right);
	s.width(width);
	s.fill(' ');
	s << v;
	return s.str();
}

std::string add_suffix(float val)
{
	const char* prefix[] = {"B", "kB", "MB", "GB", "TB"};
	const int num_prefix = sizeof(prefix) / sizeof(const char*);
	for (int i = 0; i < num_prefix; ++i)
	{
		if (fabs(val) < 1000.f)
			return to_string(val, i==0?5:4) + prefix[i];
		val /= 1000.f;
	}
	return to_string(val, 6) + "PB";
};


