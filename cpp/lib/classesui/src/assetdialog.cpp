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

#include <qcombobox.h>
#include <qgroupbox.h>
#include <qlineedit.h>
#include <qspinbox.h>
#include <qtextedit.h>
#include <qvalidator.h>
#include <qlabel.h>

//#include "assetsetitem.h"
#include "blurqt.h"
#include "database.h"

#include "asset.h"
#include "assetdialog.h"
#include "element.h"
#include "elementstatus.h"
#include "dialogfactory.h"
#include "project.h"
#include "resinerror.h"
#include "shot.h"
#include "shotgroup.h"
#include "assettemplatecombo.h"

AssetDialog::AssetDialog( const Element & parEl, QWidget * parent )
: QDialog( parent )
, mParent( parEl )
, mAssetTemplatesEnabled( false )
, mCreateAssetTemplates( true )
, mPathTemplatesEnabled( false )
{
	setupUi( this );

	setAssetTemplatesEnabled( false );
	setPathTemplatesEnabled( false );

	AssetTypeList atl = AssetType::select();//.filter( "isTask", false );
	atl = atl.sorted( "name" );
	mTypeCombo->addItems( atl.names() );
	if( atl.size() > 0 )
		setAssetType( atl[0] );

	mAllowTimeSheetsCheck->setCheckState( Qt::PartiallyChecked );
	// because of our Max object naming structure, don't allow spaces underscores, dashs in Asset names
	mAssetNameEdit->setFocus();

	connect( mCreateButton, SIGNAL( clicked() ), SLOT( create() ) );
	connect( mTypeCombo, SIGNAL( activated( const QString & ) ), SLOT( assetTypeChanged( const QString & ) ) );
	connect( mAssetTemplatesButton, SIGNAL( clicked() ), DialogFactory::instance(), SLOT( editAssetTemplates() ) );
	connect( mPathTemplatesButton, SIGNAL( clicked() ), DialogFactory::instance(), SLOT( editPathTemplates() ) );
	connect( mAssetNameEdit, SIGNAL( textChanged( const QString & ) ), SLOT( updateStatusLabel( const QString & ) ) );

	connect( mAssetTemplateCombo, SIGNAL( templateChanged( const AssetTemplate & ) ), SLOT( assetTemplateChanged( const AssetTemplate & ) ) );
}

void AssetDialog::updateStatusLabel( const QString & text )
{
	QString str( text );
	bool valid = true;
	int pos = 0;
	if( mAssetNameEdit->validator() && mAssetNameEdit->validator()->validate( str, pos ) == QValidator::Invalid )
		valid = false;
	mNameStatusLabel->setPixmap( QPixmap( valid ? ":/images/button_ok.png" : ":/images/button_cancel.png" ) );
}

ElementList AssetDialog::created()
{
	return mCreated;
}

void AssetDialog::setAssetTemplatesEnabled( bool te )
{
	mAssetTemplatesEnabled = te;
	mAssetTemplateCombo->setVisible( te );
	mAssetTemplatesButton->setVisible( te );
	mAssetTemplateLabel->setVisible( te );
	assetTypeChanged( mTypeCombo->currentText() );
}

void AssetDialog::setPathTemplatesEnabled( bool te )
{
	mPathTemplatesEnabled = te;
	mPathTemplateCombo->setVisible( te );
	mPathTemplatesButton->setVisible( te );
	mPathTemplateLabel->setVisible( te );
	assetTypeChanged( mTypeCombo->currentText() );
}

void AssetDialog::setCreateAssetTemplates( bool cat )
{
	mCreateAssetTemplates = cat;
}

void AssetDialog::setAsset( const Element & asset )
{
	// But they do need path templates
	setPathTemplatesEnabled( true );
	setAssetType( asset.assetType() );
	mTypeCombo->setEnabled( false );
	mPathTemplateCombo->setPathTemplate( asset.pathTemplate() );
	mAssetNameEdit->setText( asset.name() );
	mAssetTemplateCombo->setAssetTemplate( asset.assetTemplate() );
	QVariant cs = asset.getValue( "allowTime" );
	mAllowTimeSheetsCheck->setCheckState( cs.isNull() ? Qt::PartiallyChecked : (cs.toBool() ? Qt::Checked : Qt::Unchecked) );

	mCreateButton->hide();
	mOKButton->setText( "&Ok" );
	mCancelButton->setText( "&Cancel" );
	
	setWindowTitle( "Edit Asset" );

	mAsset = asset;
}

void AssetDialog::assetTypeChanged( const QString & atn )
{
	AssetType at = AssetType::recordByName( atn );
	
	if( at.isRecord() && mAssetTemplatesEnabled ){
		mAssetTemplateCombo->setAssetType( at );
		mAssetTemplateCombo->setProject( mParent.project() );
		assetTemplateChanged( mAssetTemplateCombo->assetTemplate() );
	}

	if( mAssetNameEdit->text().isEmpty() || mAssetNameEdit->text() == mLastAssetType.name() )
		mAssetNameEdit->setText( at.name() );
	mLastAssetType = at;

	if( !at.nameRegExp().isEmpty() ) {
		QRegExp re( at.nameRegExp() );
		mAssetNameEdit->setValidator( re.isValid() ? new QRegExpValidator( re, this ) : 0 );
	}
}

void AssetDialog::assetTemplateChanged( const AssetTemplate & at )
{
	Element e = at.element();
	QString ct = mAssetNameEdit->text();
	if( e.isRecord() && !e.name().isEmpty() && (ct == mLastAssetType.name() || ct == mLastAssetTemplate.element().name()) )
		mAssetNameEdit->setText( e.name() );
	mLastAssetTemplate = at;
}

void AssetDialog::setAssetType( const AssetType & at )
{
	int idx = mTypeCombo->findText( at.name() );
	if( idx >= 0 )
		mTypeCombo->setCurrentIndex( idx );
	assetTypeChanged( at.name() );
}

void AssetDialog::create()
{
	Database::current()->beginTransaction( "Create/Edit Asset" );

	QString name = mAssetNameEdit->text();
	if( name.isEmpty() ){
		ResinError::nameEmpty(this, "Asset Name");
		return;
	}

	AssetTemplate at = mAssetTemplateCombo->assetTemplate();

	Element e;
	if( mAsset.isRecord() ) {
		e = mAsset;

 	} else {
		if( mParent.project().isRecord() && Asset::recordByProjectAndName( mParent.project(), name ).isRecord() ){
			ResinError::nameTaken(this, "Asset Name");
			return;
		}

		AssetType assetType = AssetType::recordByName( mTypeCombo->currentText() );

		if( at.isRecord() && mAssetTemplatesEnabled && mCreateAssetTemplates ) {
			RecordList rl;
			e = Element::createFromTemplate( at, rl );
			ElementList el( rl );
			el.setProjects( mParent.project() );
			el.commit();
		} else {
			e = assetType.construct();
		}
		
		e.setProject( mParent.project() );
		e.setParent( mParent );
		e.setElementStatus( ElementStatus::recordByName( "New" ) );
		mCreated += e;
	}

	if( mAssetTemplatesEnabled )
		e.setAssetTemplate( at );

	{
		QVariant v(QVariant::Bool);
		Qt::CheckState cs = mAllowTimeSheetsCheck->checkState();
		if( cs != Qt::PartiallyChecked )
			v = QVariant( cs == Qt::Checked );
		e.setValue( "allowTime", v );
	}

	bool ipc = ( e.isRecord() && e.name() != name );
	e.setName( name );

	if( mPathTemplatesEnabled && e.pathTemplate() != mPathTemplateCombo->pathTemplate() ) {
		ipc = true;
		e.setPathTemplate( mPathTemplateCombo->pathTemplate() );
	}

	if( ipc )
		Element::invalidatePathCache();

	e.commit();

	Database::current()->commitTransaction();
}

void AssetDialog::accept()
{
	if( mAsset.isRecord() )
		create();
	QDialog::accept();
}

void AssetDialog::reject()
{
	mCreated.remove();
	QDialog::reject();
}
