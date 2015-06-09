
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of Bach.
 *
 * Bach is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Bach is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Bach; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id: qmemcached.h 5404 2007-12-18 00:01:50Z brobison $
 */

#ifndef QMEMCACHED_H
#define QMEMCACHED_H

#include <QByteArray>
#include <QMap>
#include "memcached.h"

typedef QMap<QString, QByteArray> QMemCachedResult;

class QMemCached
{
public:
	QMemCached();
	~QMemCached();

	void addServer( const QString & );

	QByteArray get(const QString & key);
	QMemCachedResult mget( const QStringList & keys );
	void set(const QString & key, QByteArray value);
	memcached_return rc();

private:
	memcached_st * mMemCache;
	memcached_server_st * mServers;
	int mServerCount;
	memcached_return mRc;
};

#endif // QMEMCACHED_H

