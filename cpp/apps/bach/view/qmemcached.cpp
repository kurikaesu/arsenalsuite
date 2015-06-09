 
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
 * $Id: qmemcached.cpp 7053 2008-09-09 08:36:27Z brobison $
 */

#include <QString>
#include <QStringList>
#include "qmemcached.h"

QMemCached::QMemCached()
{
	mMemCache = memcached_create(NULL);
	mServers = NULL;
}

QMemCached::~QMemCached()
{
  memcached_free(mMemCache);
	memcached_server_list_free(mServers);
}

void QMemCached::addServer(const QString & host)
{
	mServers = memcached_servers_parse(host.toLocal8Bit().data());
	mRc = memcached_server_push(mMemCache, mServers);
	mServerCount++;
}

QByteArray QMemCached::get(const QString & key)
{
	uint32_t flags = 0;
	size_t return_value_length = 0;
	size_t data_len = key.size();
	QByteArray ret =  memcached_get(mMemCache,
																	key.toLocal8Bit().data(),
																	data_len,
                                  &return_value_length,
																	&flags,
																	&mRc);
	return ret;
}

QMemCachedResult QMemCached::mget(const QStringList & keyList)
{
	QList<QByteArray> c_strs;
	foreach( QString i, keyList )
		c_strs.append( i.toLocal8Bit() );

	char ** c_str_list = new char*[c_strs.size()];
	size_t * keys_len = new size_t[c_strs.size()];
	for( int i=0; i< c_strs.size(); i++ ) {
		c_str_list[i] = c_strs[i].data();
		keys_len[i] = c_strs[i].size();
	}

/*
	char * keys[keyList.size()];// = {};
	size_t keys_len[keyList.size()];// = {};
	int i = 0;
	foreach( QString key, keyList ) {
		keys[i] = key.toLocal8Bit().data();
		keys_len[i] = key.size();
		i++;
	}
*/
	mRc = memcached_mget(mMemCache,
											c_str_list,
											keys_len,
											keyList.size());

	size_t return_value_length = 0;
	size_t return_key_length = 0;
	char return_key[MEMCACHED_MAX_KEY];
	uint32_t flags = 0;
	char * ret_value;
	QMemCachedResult ret;
	if( mRc == MEMCACHED_SUCCESS ) {
		while ((ret_value = memcached_fetch(
																		mMemCache,
																		return_key,
																		&return_key_length,
																		&return_value_length,
																		&flags,
																		&mRc))!=NULL)
		{
			ret[QString::fromLocal8Bit(return_key)] = QByteArray(ret_value);
		}
	}

	delete [] c_str_list;
	delete [] keys_len;

	return ret;
}


void QMemCached::set(const QString & key, QByteArray value)
{
	size_t key_len = key.size();
	char * data = value.data();
	size_t value_len = value.size();
	mRc = memcached_set(mMemCache,
											key.toLocal8Bit().data(),
											key_len,
											data,
											value_len,
											(time_t)0, (uint32_t)0);
}

memcached_return QMemCached::rc()
{
	return mRc;
}

