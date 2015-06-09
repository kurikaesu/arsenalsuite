
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of Snafu.
 *
 * Snafu is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Snafu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Snafu; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id: mainwindow.cpp 5407 2007-12-18 00:07:29Z brobison $
 */

#include <qapplication.h>
#include <qaction.h>
#include <qlabel.h>
#include <qstatusbar.h>
#include <qtextstream.h>
#include <qtooltip.h>
#include <qmenubar.h>
#include <QToolBar>

#include <stdlib.h>

#include "mainwindow.h"
#include "snafuwidget.h"
#include "blurqt.h"
#include "svnrev.h"

MainWindow::MainWindow( QWidget * parent )
: QMainWindow( parent ) 
{ 
  /* Setup counter */
	mCounterLabel = new QLabel("", statusBar()); 
	mCounterLabel->setAlignment( Qt::AlignCenter );
	mCounterLabel->setMinimumWidth(350);
	mCounterLabel->setMaximumHeight(20);
	statusBar()->addPermanentWidget( mCounterLabel, 0 );

  /* Set up MainWindow stuff */
  setWindowTitle("Snafu - Version " + VERSION + ", build " + SVN_REVSTR);

  mSW = new SnafuWidget( this );
  mSW->show();
  setCentralWidget( mSW );

	connect(mSW, SIGNAL(showStatusBarMessage(const QString &)), statusBar(), SLOT(showMessage( const QString & )));
	connect(mSW, SIGNAL(clearStatusBar()), statusBar(), SLOT(clearMessage()));

	HostToolBar = new QToolBar( this );
	HostToolBar->setIconSize(QSize(24,24));
	addToolBar(HostToolBar);
	HostToolBar->addAction( mSW->findHostAct );
	HostToolBar->addAction( mSW->refreshAct );

	IniConfig & config = ::userConfig();
  config.pushSection("SW_Display_Prefs" );
}

MainWindow::~MainWindow()
{
  delete mSW;
}

void MainWindow::closeEvent( QCloseEvent * ce )
{
  IniConfig & cfg = userConfig();
  cfg.pushSection( "MainWindow" );
  cfg.writeString( "FrameGeometry", QString("%1,%2,%3,%4").arg( pos().x() ).arg( pos().y() ).arg( size().width() ).arg( size().height() ) );
  cfg.popSection();
  QMainWindow::closeEvent(ce);
}

