
/*
 *
 * Copyright 2008 Blur Studio Inc.
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

#ifndef COMMIT_CODE

#include <qstringlist.h>
#include <qfileinfo.h>
#include <qdir.h>

#include "jobfusionvideomaker.h"

QStringList JobFusionVideoMaker::outputFormats()
{
	return QStringList() << "AVI" << "QuickTime";
}

QStringList JobFusionVideoMaker::outputCodecs(const QString & format)
{
	QStringList ret;
	if( format == "AVI" )
		ret << "Cinepack" << "Indeo Video R3.2" << "Indeo Video R4.5" << "Indeo Video R5.10" << "Intel IYUV"
		 << "Microsoft Video 1" << "DivX 6.0" << "Techsmith Screen Capture" << "Uncompressed";
	if( format == "QuickTime" )
		ret << "Sorenson Video" << "Sorenson Video 3" << "MPEG-4 Video" << "Cinepack";
	return ret;
}

QString JobFusionVideoMaker::formatFromExtension(const QString & ext)
{
	QString lext = ext.toLower();
	if( lext == "avi" ) return "AVI";
	if( lext == "mov" ) return "QuickTime";
	return QString();
}

QString JobFusionVideoMaker::formatToExtension(const QString & format)
{
	QString lformat = format.toLower();
	if( lformat == "avi" ) return lformat;
	if( lformat == "quicktime" ) return "mov";
	return QString();
}

QString JobFusionVideoMaker::updatePathToFormat(const QString & path, const QString & format)
{
	QFileInfo fi(path);
	return fi.path() + QDir::separator() + fi.completeBaseName() + "." + formatToExtension(format);
}

QString JobFusionVideoMaker::format()
{
	return formatFromExtension( QFileInfo( outputPath() ).suffix() );
}

#endif // CLASS_FUNCTIONS

