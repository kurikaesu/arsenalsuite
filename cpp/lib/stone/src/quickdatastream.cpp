
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of libstone.
 *
 * libstone is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libstone is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libstone; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id$
 */

#include "quickdatastream.h"

QuickDataStream::QuickDataStream( int size )
{
	mData = QByteArray(size,0);
	init();
}

QuickDataStream::QuickDataStream( const QByteArray & data )
{
	mData = data;
	init();
}

QuickDataStream::QuickDataStream( const QDataStream & ds )
{
	QIODevice * device = ds.device();
	if( device && device->inherits( "QBuffer" ) ) {
		QBuffer * buffer = (QBuffer*)device;
		mData = buffer->buffer();
		init();
	}
}

void QuickDataStream::init()
{
	mBuffer.setBuffer(&mData);
	mBuffer.open(QIODevice::ReadWrite);
	setDevice(&mBuffer);
}

QByteArray QuickDataStream::operator<<( QuickDataStream::EndEnum ) const
{
	return mData;
}
	
QByteArray QuickDataStream::data() const
{
	return mData;
}

