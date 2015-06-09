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

#include <stdlib.h>

#include <qtimer.h>
#include "scanner.h"

#include <qdir.h>
#include <qprocess.h>

#include "asset.h"
#include "blurqt.h"
#include "database.h"
#include "element.h"
#include "freezercore.h"
#include "path.h"
#include "project.h"
#include "rangefiletracker.h"
#include "renderelements.h"
#include "resolution.h"
#include "shot.h"
#include "shotgroup.h"
#include "task.h"
#include "tasktype.h"
#include "updatemanager.h"
#include "versionfiletracker.h"

Scanner::Scanner()
{
	QTimer * scanTimer = new QTimer( this );
	connect( scanTimer, SIGNAL(timeout()), this, SLOT(scanForNewFiles()) );
	scanTimer->start( 60000, false );
	
	// Get connected to the database
	FreezerCore::instance();
	// Get connected to update manager
	UpdateManager::instance();
	
	scanForNewFiles();
}

Scanner::~Scanner()
{
}

void Scanner::scanForNewFiles( )
{
	ElementList toScan;

	ProjectList projects = Project::select();
	st_foreach( ProjectIter, pi, projects )
	{
		// go through each Asset and Shot and Task and look for new files
		Project p( *pi );
		toScan += Element( p );

		AssetList assets = p.children( Asset::type(), true ); 
		st_foreach( AssetIter, ai, assets )
			toScan += Element( *ai );

		ShotList shots = p.children( Shot::type(), true );
		st_foreach( ShotIter, si, shots )
			toScan += Element( *si );

		TaskList tasks = p.children( Task::type(), true );
		st_foreach( TaskIter, ti, tasks )
			toScan += Element( *ti );
			
		// Find the edl file
		FileTracker ft = p.tracker( "edl_premiere" );
		bool runEDLImport = false;
		
		// Check for updated edl file
		if( ft.isRecord() && ft.lastUpdated() > p.lastScanned() )
			runEDLImport = true;
			
		// Else try to find a new edl file
		else if( !ft.isRecord() ) {
			QDir pd( p.path() );
			
			// Look in the E|editorial folder
			QStringList ed_dir_list = pd.entryList( "*ditorial", QDir::Dirs );
			st_foreach( QStringList::Iterator, it, ed_dir_list ) {
			
				// Should be here, if not they should set in resin
				QString path = p.path() + *it + "/Master_Edit/EDL/";
				QStringList edl_file_list = QDir( path ).entryList( "*.EDL" , QDir::Files, QDir::Time );
				if( !edl_file_list.isEmpty() ) {
					ft.setPath( path );
					ft.setFileName( edl_file_list[0] );
					ft.setName( "edl_premiere" );
					ft.setElement( p );
					ft.commit();
					runEDLImport = true;
					break;
				}
			}
		}
		
		if( runEDLImport ) {
			qWarning( "Running perl EDL import script on file: " + ft.filePath() );
			QStringList args;
			args << "-IEDLGeek" << "-MEDLGeek";
			args << QString( "-e'EDLGeek->exportResin( file => \"%1\", fkeyProject=>%2'" ).arg( ft.filePath() ).arg( p.key() );
			QProcess::startDetached( "perl", args );
		}
	}
	st_foreach( ElementIter, ei, toScan )
		(*ei).accountForAllFiles();
}

