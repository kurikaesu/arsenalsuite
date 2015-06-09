
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

#include <qprocess.h>
#include <qapplication.h>
#include <qprogressbar.h>
#include <qpushbutton.h>
#include <qglobal.h>
#include <qlabel.h>
#include <qmessagebox.h>
#include <qgroupbox.h>
#include <qmenu.h>
#include <qevent.h>
#include <qfile.h>
#include <qfileinfo.h>

#include "blurqt.h"

#include "recordsupermodel.h"
#include "jobtype.h"

#include "abstractdownload.h"
#include "maindialog.h"
#include "settingsdialog.h"
#include "spooler.h"

const char * start = "Start The Burn";
const char * stop = "Stop The Burn";

MainDialog::MainDialog(Slave * s, QWidget * parent)
: QDialog( parent )
, mSlave( s )
, mTrayIcon( 0 )
, mTrayMenu( 0 )
, mTrayMenuToggleAction( 0 )
, mBringToTop( true )
{
	setupUi(this);
	readConfig();

	connect( mSlave, SIGNAL( statusChange( const QString & ) ), SLOT( setStatus( const QString & ) ) );

	mActiveAssignmentsGroup->hide();

	QImage tray_image( ":images/"+cAppName+"Icon.png" );
	mTrayMenu = new QMenu( this );
	mTrayMenuToggleAction = mTrayMenu->addAction( QString( start ), this, SLOT( slotDisablePressed() ) );
	mTrayMenu->addAction( "Exit "+cAppName, qApp, SLOT( quit() ) );

	if( !tray_image.isNull() ) {
		QIcon tray_icon( QPixmap::fromImage( tray_image.scaled( 20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation ) ) );
		mTrayIcon = new QSystemTrayIcon( tray_icon, this );
		mTrayIcon->setContextMenu( mTrayMenu );
		connect( mTrayIcon, SIGNAL( activated( QSystemTrayIcon::ActivationReason ) ),
			SLOT( slotTrayIconActivated( QSystemTrayIcon::ActivationReason ) ) );
		mTrayIcon->show();
	}
	QString logo_image( ":images/"+cAppName+"Logo.png" );
	mImageLabel->setPixmap(QPixmap(logo_image));

	connect( mAFButton, SIGNAL( clicked() ), SLOT( showAssfreezer() ) );
	connect( OptionsButton, SIGNAL( clicked() ), SLOT( showOptions() ) );
	connect( ClientLogButton, SIGNAL( clicked() ), SLOT( showClientLog() ) );
	connect( mShowAssignmentsButton, SIGNAL( toggled( bool ) ), SLOT( slotShowAssignments( bool ) ) );
	connect( DisableButton, SIGNAL( clicked() ), SLOT( slotDisablePressed() ) );
	
	setWindowFlags( Qt::Window | Qt::MSWindowsFixedSizeDialogHint );
	setAttribute( Qt::WA_QuitOnClose, true );

	layout()->setSizeConstraint(QLayout::SetFixedSize);
}

MainDialog::~MainDialog()
{
	delete mSlave;
	mSlave = 0;
}

void MainDialog::setDisplay( const QString & button, const QString & status, const QString & color )
{
	QString statusText = QString("<p align=\"center\">Status [ <font size=\"+1\" color=\"%1\">%2</font> ]</p>").arg( color ).arg( status );
	mStatusLabel->setText( statusText );
	setWindowTitle(QString(cAppName+" Client [ %1 ] - Version %2, build %3").arg(status).arg(VERSION).arg(QString("$Date$").remove(QRegExp("[^\\d]"))) );
	DisableButton->setText( button );
}

void MainDialog::updateTray()
{
	bool isOffline = mSlave->status() == "offline";
	mTrayMenuToggleAction->setText( QString( isOffline ? start : stop ) );
}

void MainDialog::keyPressEvent( QKeyEvent * ke )
{
	if (ke->key() == Qt::Key_Escape) {
		// go offline, minimize to the system try
		mSlave->offline();
		hide();
	} else {
		QDialog::keyPressEvent( ke );
	}
}


typedef struct {
	const char * status, * button_text, * color;
	bool bring_to_front;
} display;

display disps [] =
{
	{ "Offline", start, "#000000", false },
	{ "Ready", stop, "#008800", true },
	{ "Copy", stop, "#DD8822", true },
	{ "Assigned", stop, "#DD8822", true },
	{ "Busy", stop, "#DD8822", true },
    { "Maintenance", start, "#E16464", true },
	{ "Error", start, "#000000", true },
	{ 0, 0, 0, false }
};

void MainDialog::setStatus( const QString & status )
{
	updateTray();
	LOG_5( "MainDialog::setStatus() called with status " + status );

	QString st = status;
	st[0] = st[0].toUpper();

	// Find the status display entry
	display * d = disps;
	for( ; d && d->status; d++ )
		if( st == QString( d->status ) )
			break;

	// If not found, move back to "Error"
	if( !d->status ) d--;

	if( d && d->status ) {
		setDisplay( d->button_text, d->status, d->color );
		if( d->bring_to_front ) {
			if( !isVisible() )
				show();
			if( mBringToTop ) {
				raise();
				activateWindow();
			}
		}
	}

	mBringToTop = false;

	LOG_5( "MainDialog::setStatus() done setting status " + status + ", returning");
}

void MainDialog::showOptions()
{
	LOG_5("MWIN: Showing settings dialog");
	SettingsDialog * settings = new SettingsDialog(this);
	// Exec for a modal dialog
	int result = settings->exec();
	
	if( result == QDialog::Accepted ){
		LOG_5("MWIN: Applying new settings");
		settings->applyChanges();	// Apply the changes to 'config'
		readConfig();				// Read the values from 'config'
	}
	delete settings;
}

void MainDialog::showClientLog()
{
	QProcess::startDetached(cLogCommand.arg(cClientLogFile));
}

void MainDialog::slotDisablePressed()
{
	mBringToTop = true;
	mSlave->toggle();
	if( mSlave->status() == "offline" )
		hide();
}

void MainDialog::readConfig()
{
	IniConfig & c( config() );
	c.pushSection( "Assburner" );
	cClientLogFile = c.readString("ClientLogFile", CLIENT_LOG );
	LOG_3( "MainDialog::readConfig setting cClientLogFile to " + cClientLogFile );
	cLogCommand = c.readString("LogCommand", LOG_COMMAND );
	LOG_3( "MainDialog::readConfig setting cLogCommand to " + cLogCommand );
	cAFPath = c.readString("AFPath", AF_COMMAND);
	LOG_3( "MainDialog::readConfig setting cAFPath to " + cAFPath );
	cAppName = c.readString("ApplicationName", "Assburner");
	c.popSection();
}

void MainDialog::showAssfreezer()
{
	if( !QFile::exists( cAFPath ) ) {
		QMessageBox::warning( this, "Assfreezer Missing", "Assfreezer could not be found at: " + cAFPath );
		return;
	}
	QProcess * p = new QProcess(this);
	// Delete Automatically when assfreezer exits
	connect( p, SIGNAL( finished ( int, QProcess::ExitStatus ) ), p, SLOT(deleteLater()));
	p->setWorkingDirectory( QFileInfo(cAFPath).path() );
	p->start( cAFPath, QStringList() << "-current-render" );
}

void MainDialog::slotShowAssignments( bool sa )
{
	if( sa ) {
		mActiveAssignmentsGroup->show();
		mShowAssignmentsButton->setText( "-" );
	} else {
		mActiveAssignmentsGroup->hide();
		mShowAssignmentsButton->setText( "+" );
		QTimer::singleShot( 0, this, SLOT( updateSize() ) );
		layout()->update();
	}
}

void MainDialog::slotAssignmentsChanged( JobAssignmentList assignments )
{
	//mModel->updateRecords( assignments );
}

void MainDialog::updateSize()
{
	resize( sizeHint() );
}

void MainDialog::closeEvent( QCloseEvent * ce )
{
	if( ce->spontaneous() ) {
		LOG_5( "MainDialog::closeEvent: Ignoring spontaneous closeevent." );
		ce->ignore();
		hide();
		return;
	}
	//
	// AFAIK, this code is not currently run
	// because we have the winEventFilter in
	// main.cpp that filters out
	// WM_QUERYENDSESSION and WM_ENDSESSION
	// messages.  There may be other ways
	// that closeEvent gets called with spontaneous==false
	// though
	LOG_5( "MainDialog::closeEvent: Deleting slave" );
	delete mSlave;
	mSlave = 0;
	delete mTrayIcon;
	mTrayIcon = 0;
	LOG_5( "MainDialog::closeEvent: Calling QCloseEvent::accept" );
	ce->accept();
	LOG_5( "MainDialog::closeEvent: returning" );
}

void MainDialog::slotTrayIconActivated( QSystemTrayIcon::ActivationReason reason )
{
	if( reason == QSystemTrayIcon::Trigger ) {
		if( isVisible() && isActiveWindow() ) {
			LOG_3( "MainDialog::slotTrayIconClicked()  Going offline" );
			mSlave->offline();
			hide();
		} else if( isVisible() ) {
			LOG_3( "MainDialog::slotTrayIconClicked()  Raising window" );
			raise();
			activateWindow();
		} else {
			LOG_3( "MainDialog::slotTrayIconClicked()  Going online" );
			// This will raise the window and set it active
			mBringToTop = true;
			mSlave->online();
		}
	}
}

