 
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of libstone.
 *
 * libstone is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libstone is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libstone; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id: configdbdialog.cpp 8976 2009-11-09 18:57:32Z newellm $
 */

#include <qlineedit.h>
#include <qspinbox.h>

#include "configdbdialog.h"
#include "iniconfig.h"
#include "blurqt.h"

ConfigDBDialog::ConfigDBDialog( QWidget * parent )
: QDialog( parent )
{
	mUI.setupUi( this );

	IniConfig & c = config();
	c.pushSection( "Database" );
	setWindowTitle( "Database Connection Settings" );
	mUI.mDBNameEdit->setText( c.readString( "DatabaseName", "blur" ) );
	mUI.mHostEdit->setText( c.readString( "Host", "lion" ) );
	mUI.mPortSpin->setValue( c.readInt( "Port", 5432 ) );
	mUI.mUserNameEdit->setText( c.readString( "User", "brobison" ) );
	mUI.mPasswordEdit->setText( c.readString( "Password", "test" ) );
	c.popSection();

	setFixedSize( size() );
}

void ConfigDBDialog::accept()
{
	IniConfig & c = config();
	c.pushSection( "Database" );
	c.writeString( "DatabaseName", mUI.mDBNameEdit->text() );
	c.writeString( "Host", mUI.mHostEdit->text() );
	c.writeInt( "Port", mUI.mPortSpin->value() );
	c.writeString( "User", mUI.mUserNameEdit->text() );
	c.writeString( "Password", mUI.mPasswordEdit->text() );
	c.popSection();
	QDialog::accept();
}

