
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
 * along with Arsenal; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id$
 */

#include <qtimer.h>
#include <qmenu.h>
#include <qtreewidget.h>

#include "seedview.h"
#include "slave.h"
#include "spooler.h"
#include "abstractdownload.h"

#ifdef LIB_TORRENT
#include "libtorrent/torrent_handle.hpp"
#include "libtorrent/session.hpp"

class SeedItem : public QTreeWidgetItem
{
public:
	SeedItem( SeedView * parent, const libtorrent::torrent_handle & th )
	: QTreeWidgetItem( parent )
	, mHandle( th )
	{
		update();
	}

	void update()
	{
		SeedView * sv = (SeedView*)treeWidget();
		Slave * slave = sv->slave();
		if( !slave ) return;
		SpoolItem * ti = slave->spooler()->itemFromHandle( mHandle );
		if( !ti ) return;
		setText( 0, ti->mJob.name() );
		libtorrent::torrent_status ts = mHandle.status();
		setText( 1, add_suffix( ts.total_wanted ).c_str() );
		setText( 2, QString(add_suffix( ts.upload_rate ).c_str()) + "/s "
					+ QString(add_suffix( ts.download_rate ).c_str()) + "/s" );
		setText( 3, QString(add_suffix( ts.total_upload ).c_str()) + " " + add_suffix( ts.total_download ).c_str() );
	}

	libtorrent::torrent_handle mHandle;
};
#endif // LIB_TORRENT

SeedView::SeedView( QWidget * parent )
: QTreeWidget( parent )
, mSlave( 0 )
{
	QStringList columns;
	columns << "Job" << "Size" << "Rate(Up/Down)" << "Total(Up/Down)";
	setHeaderLabels( columns );

	mUpdateTimer = new QTimer( this );
	connect( mUpdateTimer, SIGNAL( timeout() ), SLOT( update() ) );

	connect( this, SIGNAL( customContextMenuRequested( const QPoint & ) ), SLOT( showContext( const QPoint & ) ) );
	setContextMenuPolicy( Qt::CustomContextMenu );

	setMinimumHeight( 180 );
}

void SeedView::setSlave( Slave * slave )
{
	mSlave = slave;
}

Slave * SeedView::slave()
{
	return mSlave;
}

void SeedView::update()
{
	if( !mSlave ) return;

	clear();
#ifdef LIB_TORRENT
	mItemMap.clear();

	std::vector<libtorrent::torrent_handle> handles = TorrentManager::instance()->torrentSession()->get_torrents();
	for (std::vector<libtorrent::torrent_handle>::iterator i = handles.begin(); i != handles.end(); ++i)
	{
		libtorrent::torrent_handle th = *i;
		mItemMap[th] = new SeedItem( this, th );
	}
#endif // LIB_TORRENT
}

void SeedView::showEvent ( QShowEvent * )
{
	mUpdateTimer->start( 1000 );
}

void SeedView::hideEvent ( QHideEvent * )
{
	mUpdateTimer->stop();
}

void SeedView::showContext( const QPoint &  )
{
#ifdef LIB_TORRENT
	SeedItem * si = (SeedItem*)currentItem();
	if( si ) {
		QMenu * mnu = new QMenu( this );
		QAction * rem = mnu->addAction( "Remove Seed" );
		QAction * fr = mnu->addAction( "Force Reannounce" );
		QAction * res = mnu->exec( QCursor::pos() );
		if( res == rem ) {
			TorrentManager::instance()->torrentSession()->remove_torrent( si->mHandle );
		} else if( res == fr ) {
			si->mHandle.force_reannounce();
		}
	}
#endif // LIB_TORRENT
}

