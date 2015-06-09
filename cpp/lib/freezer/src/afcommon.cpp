
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
 * along with Blur; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* $Header$
 */

#include <qdir.h>
#include <qimage.h>
#include <qfileinfo.h>
#include <qprocess.h>

#include "afcommon.h"

#include "path.h"

#ifdef Q_OS_WIN

const char  * LOGO_PATH = "images\\AssFreezerLogo.jpg",
			* CONFIG_PATH = "\\Application Data\\assfreezer.cfg",
			* FRAGMENT_SHADER_PATH = "fragment_shader.cg",
			* ICON_PATH = "images\\";
const char * VNC_LINK = 
	"[connection]\nhost=%1\nport=5900\npassword=62d0bc9c593df2d5\n[options]\nuse_encoding_0=1\nuse_encoding_1=1\n"
	"use_encoding_2=1\nuse_encoding_3=0\nuse_encoding_4=1\nuse_encoding_5=1\nuse_encoding_6=1\nuse_encoding_7=1\n"
	"use_encoding_8=1\npreferred_encoding=7\nrestricted=0\nviewonly=0\nfullscreen=0\n8bit=0\nshared=1\nswapmouse=0\n"
	"belldeiconify=0\nemulate3=1\nemulate3timeout=100\nemulate3fuzz=4\ndisableclipboard=0\nlocalcursor=1\nscale_den=1\n"
	"scale_num=1\ncursorshape=1\nnoremotecursor=0\ncompresslevel=8\nquality=0\n";


#else // Q_OS_WIN

const char  * LOGO_PATH = ":/images/ArsenalFreezerLogo.jpg",
			* CONFIG_PATH = "/.assfreezer",
			* FRAGMENT_SHADER_PATH = "/usr/share/assfreezer/fragment_shader.cg",
			* ICON_PATH = "/usr/share/assfreezer/";

#endif // !Q_OS_WIN

void exploreFile( QString path )
{
	QFileInfo fi(path);
	QStringList args;
	QString dirPath = QFileInfo( path ).path();

	QString cmd;
	#ifdef Q_OS_WIN
	cmd = "explorer.exe";
	if( fi.isFile() && fi.exists() ) {
		args << "/select,";
		args << path.replace("/","\\");
	} else
		args << dirPath.replace("/", "\\");
	#endif
	#ifdef Q_OS_LINUX
	args << dirPath.replace('\\', '/');
	cmd = "thunar";
	#endif
	#ifdef Q_OS_MAC
	args << dirPath.replace('\\', '/');
	cmd = "open";
	#endif
	if( !QProcess::startDetached( cmd, args ) ) {
		LOG_1( "Unable to start " + cmd );
	}
}

