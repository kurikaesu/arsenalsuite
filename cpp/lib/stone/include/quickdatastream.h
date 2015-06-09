
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

#ifndef QUICK_DATA_STREAM_H
#define QUICK_DATA_STREAM_H

#include <qdatastream.h>
#include <qbytearray.h>
#include <qbuffer.h>

#include "blurqt.h"

/**
 *  This class is a convenience class to provide shorter code to
 *  create a QByteArray and fill it with some binary data.
 *
 *  Pure Qt Code:
 *
 *   QByteArray bytes;
 *   QDataStream ds(&bytes,QIODevice::WriteOnly);
 *   ds << data1 << data2;
 *   callFunction(bytes);
 *
 *  With QuickDataStream
 *	typedef QuickDataStream QDS;
 *	callFunction(QDS() << data1 << data2 << QDS::End)
 **/

class STONE_EXPORT QuickDataStream : public QDataStream
{
public:
	QuickDataStream( int size = 0 );
	QuickDataStream( const QByteArray & );
	QuickDataStream( const QDataStream & );

	enum EndEnum { End };

	QuickDataStream & operator<< ( qint8 i ) { QDataStream::operator<<(i); return *this; }
	QuickDataStream & operator<< ( bool i )  { QDataStream::operator<<(i); return *this; }
	QuickDataStream & operator<< ( quint8 i )  { QDataStream::operator<<(i); return *this; }
	QuickDataStream & operator<< ( quint16 i )  { QDataStream::operator<<(i); return *this; }
	QuickDataStream & operator<< ( qint16 i )  { QDataStream::operator<<(i); return *this; }
	QuickDataStream & operator<< ( qint32 i )  { QDataStream::operator<<(i); return *this; }
	QuickDataStream & operator<< ( quint64 i )  { QDataStream::operator<<(i); return *this; }
	QuickDataStream & operator<< ( qint64 i )  { QDataStream::operator<<(i); return *this; }
	QuickDataStream & operator<< ( quint32 i )  { QDataStream::operator<<(i); return *this; }
	QuickDataStream & operator<< ( float f )  { QDataStream::operator<<(f); return *this; }
	QuickDataStream & operator<< ( double f )  { QDataStream::operator<<(f); return *this; }
	QuickDataStream & operator<< ( const char * s )  { QDataStream::operator<<(s); return *this; }
	QuickDataStream & operator<< ( const QByteArray & ba ) { mBuffer.write(ba); return *this; }

	QuickDataStream & operator>> ( qint8 & i )  { QDataStream::operator>>(i); return *this; }
	QuickDataStream & operator>> ( bool & i )  { QDataStream::operator>>(i); return *this; }
	QuickDataStream & operator>> ( quint8 & i )  { QDataStream::operator>>(i); return *this; }
	QuickDataStream & operator>> ( quint16 & i )  { QDataStream::operator>>(i); return *this; }
	QuickDataStream & operator>> ( qint16 & i )  { QDataStream::operator>>(i); return *this; }
	QuickDataStream & operator>> ( quint32 & i )  { QDataStream::operator>>(i); return *this; }
	QuickDataStream & operator>> ( qint32 & i )  { QDataStream::operator>>(i); return *this; }
	QuickDataStream & operator>> ( quint64 & i )  { QDataStream::operator>>(i); return *this; }
	QuickDataStream & operator>> ( qint64 & i )  { QDataStream::operator>>(i); return *this; }
	QuickDataStream & operator>> ( float & f )  { QDataStream::operator>>(f); return *this; }
	QuickDataStream & operator>> ( double & f )  { QDataStream::operator>>(f); return *this; }
	QuickDataStream & operator>> ( char *& s ) { QDataStream::operator>>(s); return *this; }

	QByteArray operator<<( QuickDataStream::EndEnum ) const;
	
	QByteArray data() const;

protected:
	void init();

	QByteArray mData;
	QBuffer mBuffer;
};

#endif // QUICK_DATA_STREAM_H

