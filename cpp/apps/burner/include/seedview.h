
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

#ifndef SEED_VIEW_H
#define SEED_VIEW_H

#include <qtreewidget.h>
#include <qmap.h>

#ifdef LIB_TORRENT
#include "libtorrent/torrent_handle.hpp"
#endif // LIB_TORRENT

class SeedItem;
class Slave;
class QTimer;

/// \ingroup ABurner
/// @{

class SeedView : public QTreeWidget
{
Q_OBJECT
public:
	SeedView( QWidget * parent = 0 );

	Slave * slave();
	void setSlave( Slave * );

public slots:
	void update();
	void showContext( const QPoint & );

protected:

	virtual void showEvent ( QShowEvent * );
	virtual void hideEvent ( QHideEvent * );

	QTimer * mUpdateTimer;
	Slave * mSlave;

#ifdef LIB_TORRENT
	typedef QMap<libtorrent::torrent_handle,SeedItem*> TorrentItemMap;
	typedef QMap<libtorrent::torrent_handle,SeedItem*>::Iterator TorrentItemIter;
	TorrentItemMap mItemMap;
#endif // LIB_TORRENT
};

/// @}

#endif // SEED_VIEW_H

