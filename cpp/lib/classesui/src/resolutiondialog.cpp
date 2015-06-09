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
#include <qfiledialog.h>
#include <qlineedit.h>
#include <qmessagebox.h>
#include <qspinbox.h>
#include <qpushbutton.h>

#include "resolutiondialog.h"
#include "fieldspinbox.h"
#include "path.h"
#include "user.h"

ResolutionDialog::ResolutionDialog( QWidget * parent )
: QDialog( parent )
{
	setupUi( this );
	connect( mFillFrameImageButton, SIGNAL( clicked() ), SLOT( setFillFrameImage() ) );
	mPixelAspectEdit->setText( "1.0" );
}

ProjectResolution ResolutionDialog::resolution()
{
	return mResolution;
}

void ResolutionDialog::setResolution( const ProjectResolution & r )
{
	mResolution = r;
	mNameEdit->setText( r.name() );
	mWidthSpin->setValue( r.width() );
	mHeightSpin->setValue( r.height() );
	int idx = mDeliveryFormatCombo->findText( r.deliveryFormat() );
	if( idx >= 0 )
		mDeliveryFormatCombo->setCurrentIndex( idx );
	idx = mOutputFormatCombo->findText( r.outputFormat() );
	if( idx >= 0 )
		mOutputFormatCombo->setCurrentIndex( idx );

	mPixelAspectEdit->setText( QString::number( (r.pixelAspect()>0) ? r.pixelAspect() : 1.0 ) );
	mFPSSpin->setValue( r.fps() > 0 ? r.fps() : 30 );
	
	bool hasPerms = User::hasPerms( "Resolution", true );
	mNameEdit->setReadOnly( !hasPerms );
	mWidthSpin->setEnabled( hasPerms );
	mHeightSpin->setEnabled( hasPerms );
	mDeliveryFormatCombo->setEnabled( hasPerms );
	mOutputFormatCombo->setEnabled( hasPerms );
	mOKButton->setEnabled( hasPerms );
	setWindowTitle( hasPerms ? "Edit Resolution" : "View Resolution(ReadOnly)" );
}

void ResolutionDialog::setFillFrameImage()
{
	Path p( QFileDialog::getOpenFileName( this, "Images (*.png *.jpeg *.jpg *.tga)", 
		"Open fill frame image", "Select image for filling empty frames" ) );
	if( p.fileExists() )
		p.copy( mResolution.fillFrameFilePath() );
}

void ResolutionDialog::accept()
{
	if( mResolution.isRecord()
		&& ( (uint)mWidthSpin->value() != mResolution.width() || (uint)mHeightSpin->value() != mResolution.height() ) 
		&& QMessageBox::warning( this,
		"Warning: Changing resolution causes changes to directory structure",
		"Changing the resolution will cause all of the renderOutput and compOutput"
		" folders to be moved. This could break current renders and fusion flows."
		, QMessageBox::Ok, QMessageBox::Cancel ) == QMessageBox::Cancel )
		return;
	
	mResolution.setName( mNameEdit->text() );
	mResolution.setWidth( mWidthSpin->value() );
	mResolution.setHeight( mHeightSpin->value() );
	mResolution.setDeliveryFormat( mDeliveryFormatCombo->currentText() );
	mResolution.setOutputFormat( mOutputFormatCombo->currentText() );
	mResolution.setPixelAspect( mPixelAspectEdit->text().toFloat() );
	mResolution.setFps( mFPSSpin->value() );
	
	QDialog::accept();
}

