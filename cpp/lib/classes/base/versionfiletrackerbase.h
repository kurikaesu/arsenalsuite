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

#include <qstringlist.h>
class ElementList;
#endif

#ifdef CLASS_FUNCTIONS
	QString fileName() const;
	bool setFileName( const QString & );
	
	QString path() const;
	void setPath( const QString & );
	
	void checkForUpdates();
	
	bool doesTrackFile( const QString & );
	
	QStringList fileNames() const;

	QStringList oldFileNames() const;
	void setOldFileNames( const QStringList & );

	bool copyToFinal( QString * errorMsg=0 ) const;
	QString finalPath() const;
	QString finalFilePath() const;
	
	QString getFinalFileName() const;
	
	uint version() const;
	void setVersion( uint version );
	
	uint iteration() const;
	void setIteration( uint iteration );
	
	bool autoFinal() const;
	void setAutoFinal( bool autoFinal );
	
	QString fileNameTemplate() const;
	void setFileNameTemplate( const QString & );
	
	bool isLinked() const;
	//
	// Parses the fileName to get the version and iteration
	// Only returns true if the filename matches this fileversion's
	// prefix and if the version and iteration can be parsed out
	bool parseVersion( const QString & fileName, int * version=0, int * iteration=0 );

	QString assembleVersion( int version, int iteration );
	
	// Returns all of the elements that are using this
	// file tracker. (Linked filetrackers' elements, and
	// the one non-linked filetracker's element).
	ElementList elements() const;
	
	// Returns all the elements for the tracker
	// that tracks 'file'
	static ElementList elementsFromPath( const QString & file );

#endif

