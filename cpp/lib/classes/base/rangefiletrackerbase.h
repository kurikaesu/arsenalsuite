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

#ifdef HEADER_FILES

#include <qstring.h>
#include <qstringlist.h>
class ElementList;
class RFTFrame;
#endif


#ifdef CLASS_FUNCTIONS
	
	QString fileName() const;
	bool setFileName( const QString & );
	void checkForUpdates();
	bool doesTrackFile( const QString & filePath );
	QDateTime updated( bool last = true ) const;
	
	QStringList files() const;

//	RFTFrame firstFrame() const;
//	RFTFrame lastFrame() const;
//	RFTFrame frame( int ) const;
		
	QString sortString( int frame ) const;

	QString displayNumber( int frame ) const;

	QString filePath( int frame ) const;
	QString fileName( int frame ) const;
	void setFilePath( const QString & );

	void fillFrames( bool overwrite=false );
	
	void fillFrame( int frame ) const;
	
	static QString timeCode( int frame, int fps );

	void deleteFrame( int frame ) const;

	
#endif

