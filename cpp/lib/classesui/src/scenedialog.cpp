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

#include <qlineedit.h>
#include <qspinbox.h>
#include <qtextedit.h>

#include "database.h"

#include "assettype.h"
#include "scenedialog.h"
#include "shotgroup.h"

SceneDialog::SceneDialog( QWidget * parent )
: QDialog( parent )
{
	setupUi( this );
}

ShotGroup SceneDialog::shotGroup()
{
	mShotGroup.setName( mNameEdit->text() );
	mShotGroup.setElementType( ShotGroup::type() );
	mShotGroup.setAssetType( AssetType::recordByName( "Scene" ) );
	return mShotGroup;
}

void SceneDialog::setShotGroup( ShotGroup shotGroup )
{
	if( shotGroup.isRecord() )
		setWindowTitle( "Edit Scene" );
	mShotGroup = shotGroup;
	mNameEdit->setText( shotGroup.name() );
}

void SceneDialog::accept()
{
	ShotGroup scene = shotGroup();
	Database::current()->beginTransaction( scene.isRecord() ? "Edit Scene" : "Create Scene" );
	scene.commit();
	Database::current()->commitTransaction();
	QDialog::accept();
}


