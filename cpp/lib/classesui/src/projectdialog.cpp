/*
 *
 * Copyright 2003, 2004 Blur Studio Inc.
 *
 * This file is part of the Resin software package.
 *
 * Resin is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Resin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Resin; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qdatetime.h>
#include <qdatetimeedit.h>
#include <qgroupbox.h>
#include <qinputdialog.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qstringlist.h>
#include <qstackedwidget.h>

#include "assettemplate.h"
#include "assettype.h"
#include "blurqt.h"
#include "client.h"
#include "config.h"
#include "dialogfactory.h"
#include "group.h"
#include "elementstatus.h"
#include "project.h"
#include "projectdialog.h"
#include "projectstatus.h"
#include "resinerror.h"
#include "shotgroup.h"
//#include "taskswidget.h"
#include "user.h"
#include "usergroup.h"
#include "host.h"

ProjectDialog::ProjectDialog( QWidget * parent )
: QDialog( parent )
{
	setupUi( this );
	mDueDate->setDate( QDate::currentDate() );
	mStartDate->setDate( QDate::currentDate() );

	mTemplatesCombo->setAssetType( AssetType::recordByName( "Project" ) );

	QStringList hosts = Host::select().names();
	hosts.sort();
	refreshClientList();
	
	mCompOutputCombo->addItems( Config::recordByName( "compOutputDrives" ).value().split(',') );
	mRenderOutputCombo->addItems( Config::recordByName( "renderOutputDrives" ).value().split(',') );
	mWIPCombo->addItems( Config::recordByName( "wipOutputDrives" ).value().split(',') );
	
	connect( mNewClientButton, SIGNAL( clicked() ), SLOT( slotNewClient() ) );
	connect( mNextButton, SIGNAL( clicked() ), SLOT( slotNext() ) );
	connect( mPrevButton, SIGNAL( clicked() ), SLOT( slotPrev() ) );
	mOKButton->hide();
	mPrevButton->hide();
	connect( mEditTemplatesButton, SIGNAL( clicked() ), DialogFactory::instance(), SLOT( editAssetTemplates() ) );
}

void ProjectDialog::refreshClientList()
{
	QStringList clientNames = Client::select().names();
	clientNames.sort();
	mClientCombo->clear();
	mClientCombo->addItem( "None" );
	mClientCombo->addItems( clientNames );
}

void ProjectDialog::slotNewClient()
{
	bool ok;
	QString name = QInputDialog::getText( this, "New Client", "Enter Client Name:", QLineEdit::Normal, QString::null, &ok );
	if( ok && !name.isEmpty() ) {
		Client c;
		c.setName( name );
		c.commit();
		refreshClientList();
	}
}

void ProjectDialog::slotPrev()
{
	QWidget * vw = mStack->currentWidget();
	if( vw == mPageTwo ) {
		mStack->setCurrentWidget( mPageOne );
		mPrevButton->hide();
		mNextButton->show();
		mOKButton->hide();
	} else
		LOG_5( "ProjectDialog::slotPrev(), error - should be mPageOne or mPageTwo" );

}

void ProjectDialog::slotNext()
{
	QWidget * vw = mStack->currentWidget();
	if( vw == mPageOne ) {
		Project p = project();
		if( !p.isRecord() ){
			if( p.name().isEmpty() ){
				ResinError::nameEmpty( this, "Project Name" );
				return;
			}
			if( Project::recordByName( p.name() ).isRecord() ){
				ResinError::nameTaken( this, p.name() );
				return;
			}
			if( p.shortName().isEmpty() ){
				ResinError::nameEmpty( this, "Abbreviation" );
				return;
			}
		}
		mStack->setCurrentWidget( mPageTwo );
		mPrevButton->show();
		mOKButton->show();
		mNextButton->hide();
	} else
		LOG_5( "ProjectDialog::slotNext(), error - should be mPageOne or mPageTwo" );
}

ProjectStorage createStorage( const Project & p, const QString & name, const QString & loc )
{
	ProjectStorage ret;
	ret.setProject( p );
	ret.setName( name );
	ret.setLocation( loc );
	return ret;
}

void ProjectDialog::accept()
{
	Project p = project();
	bool ufc = p.useFileCreation();
	p.setUseFileCreation( 0 );
	p.setElementStatus( ElementStatus::recordByName( "New" ) );
	p.setProjectStatus( ProjectStatus::recordByName( "Production" ) );
	p.commit();
	p.setUseFileCreation( ufc );
	p.setProject( p );
	
	Client client( Client::recordByName( mClientCombo->currentText() ) );
	p.setClient( client );
	p.commit();
	
	if( mWebCheck->isChecked() || mFTPCheck->isChecked() ) {
		User u = User::setupProjectUser( p, client, mWebCheck->isChecked(), mFTPCheck->isChecked() );
		u.commit();
	}
	
	/* Create Project Storage Records */
	ProjectStorageList storage;
	storage += createStorage( p, "animation", mWIPCombo->currentText() );
	storage += createStorage( p, "renderOutput", mRenderOutputCombo->currentText() );
	storage += createStorage( p, "compOutput", mCompOutputCombo->currentText() );
	storage.commit();

	QDialog::accept();
}

Project ProjectDialog::project()
{
	RecordList rl;
	AssetTemplate at = mTemplatesCombo->assetTemplate();
	Project p;
	if( at.isRecord() )
		p = Element::createFromTemplate( at, rl );
	else
		p = AssetType::recordByName( "Project" ).construct();

	ElementList el( rl );
	el.setProjects( p );
	el.commit();

	p.setName( mProjectNameEdit->text() );
	p.setStartDate( mStartDate->date() );
	p.setDueDate( mDueDate->date() );
	p.setShortName( mAbbrEdit->text() );
	p.setUseFileCreation( mAutoCreateCheck->isChecked() ? 1 : 0 );
	p.setRenderOutputDrive( mRenderOutputCombo->currentText() );
	p.setCompOutputDrive( mCompOutputCombo->currentText() );
	p.setWipDrive( mWIPCombo->currentText() );
	//p.setFilePath( mWIPCombo->currentText() + "/" + mProjectNameEdit->text() );
	p.setElementType( Project::type() );
	return p;
}

