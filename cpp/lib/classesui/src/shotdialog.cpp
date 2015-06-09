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

#include <qfile.h>
#include <qfiledialog.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qmap.h>
#include <qmessagebox.h>
#include <qspinbox.h>
#include <qvalidator.h>

#include "blurqt.h"
#include "fieldspinbox.h"
#include "project.h"
//#include "resinerror.h"
#include "shot.h"
#include "shotdialog.h"
#include "database.h"
//#include "taskswidget.h"
#include "thumbnail.h"
#include "elementstatus.h"
//#include "kernel.h"

ShotDialog::ShotDialog( const Element & element, QWidget * parent )
: QDialog( parent )
, mElement( element )
{
	setupUi( this );

	mShotStart->setValidator( new QDoubleValidator( 1.0, 99999999.0, 2, mShotStart ) );
	mShotEnd->setValidator( new QIntValidator( 1, 99999999, mShotEnd ) );
	mShotName->setText( "S" );
	mShotName->setEnabled( false );
	
	mTemplateCombo->setProject( mElement.project() );
	mTemplateCombo->setAssetType( AssetType::recordByName( "Shot" ) );

	connect( mShotStart, SIGNAL( textChanged( const QString & ) ), SLOT( shotStartChange( const QString & ) ) );
	connect( mShotEnd, 	 SIGNAL( textChanged( const QString & ) ), SLOT( shotEndChange( const QString & ) ) );
	connect( mShotName,  SIGNAL( textChanged( const QString & ) ), SLOT( updateResult() ) );
//	connect( mEditTemplatesButton, SIGNAL( clicked() ), kernel, SLOT( slotEditAssetTemplates() ) );
}

void ShotDialog::setShotNumber( float sn )
{
	mShotStart->setText( QString::number(sn) );
	mShotEnd->setText( QString::number(sn) );
}

void ShotDialog::setShotName( const QString & name )
{
	mShotName->setText( name );
}

void ShotDialog::shotStartChange( const QString & value )
{
	float val = value.toDouble();
	bool isFloat = (value.indexOf(".")>=0);
	mShotEnd->setEnabled( !isFloat );
	if( isFloat )
		val = (int)(val + .99);
	if( val > mShotEnd->text().toDouble() )
		mShotEnd->setText( value );
	((QIntValidator*)mShotEnd->validator())->setBottom( (int)val );
	updateResult();
}

void ShotDialog::shotEndChange( const QString & value )
{
	((QDoubleValidator*)mShotStart->validator())->setTop( value.toDouble() );
	updateResult();
}

void ShotDialog::updateResult()
{
	QString resText("Result: ");
	resText += mShotName->text();
	Shot s;
	s.setShotNumber( mShotStart->text().toDouble() );
	s.setElementType( Shot::type() );
	resText += s.displayNumber();
	if( mShotStart->text() != mShotEnd->text() && !mShotStart->text().contains( "." ) ){
		s.setShotNumber( mShotEnd->text().toDouble() );
		resText += " to " + mShotName->text() + s.displayNumber();
	}
	mStatusLabel->setText( resText );
}

Shot ShotDialog::shotSetup()
{
	RecordList rl;
	Shot temp;
	AssetTemplate at = mTemplateCombo->assetTemplate();
	if( at.isRecord() )
		temp = Element::createFromTemplate( at, rl );
	else
		temp = AssetType::recordByName( "Shot" ).construct();
	ElementList el(rl);
	el.setProjects( mElement.project() );
	el.commit();
	LOG_5( "Shot's assettype is: " + temp.assetType().name() );
	temp.setFrameStart( mFrameStartSpin->value() );
	temp.setFrameEnd( mFrameEndSpin->value() );
	temp.setName( mShotName->text() );
	temp.setParent( mElement );
	temp.setProject( mElement.project() );
	temp.setElementStatus( ElementStatus::recordByName( "New" ) );
	temp.setElementType( Shot::type() );
	return temp;
}

ShotList ShotDialog::createdShots() const
{
	return mCreated;
}

void ShotDialog::accept()
{
	double shotStartFloat = mShotStart->text().toDouble();
	int shotStart = (int)shotStartFloat;
	int shotEnd = mShotEnd->text().toInt();

	// Check for existing shot numbers
	ShotList shotsInProject = mElement.children( Shot::type(), true );
	QMap<float, bool> existingShots;
	foreach( Shot s, shotsInProject)
		existingShots[s.shotNumber()] = true;

	if( shotStartFloat != (float)shotStart ){
		if( existingShots[shotStartFloat] == true ){
			LOG_5( "Shot exists!" );
//			ResinError::nameTaken( this, QString::number(shotStartFloat) );
			return;
		}
		Shot temp = shotSetup();
		temp.setShotNumber( shotStartFloat );
		temp.setName( temp.name() + temp.displayNumber() );
		temp.commit();
	//	mThumbnailButton->updateThumbnail( temp );
		QDialog::accept();
		return;
	}

	QStringList badShots;
	for(int shit = shotStart; shit<=shotEnd; shit++)
		if (existingShots[(float)shit] == true)
			badShots += QString::number( shit );

	if( badShots.size() > 0 ) {
		//ResinError::nameTaken( this, badShots.join(", ") );
		return;
	}
	
	ShotList shots_to_add;
	
	// Create the shots
	for(; shotStart<=shotEnd; shotStart++) {
		Shot t = shotSetup();
		t.setShotNumber( shotStart );
		t.setName( t.name() + t.displayNumber() );
		shots_to_add += t;
	}
	
	// Commit the shots
	shots_to_add.commit();
	mCreated = shots_to_add;
	// Create and insert the thumbnails
//	mThumbnailButton->updateThumbnail( shots_to_add );
	
	QDialog::accept();
}

