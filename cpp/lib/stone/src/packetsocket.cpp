
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

#include <qdatastream.h>
#include <qdatetime.h>

#include "blurqt.h"
#include "packetsocket.h"

PacketSocket::PacketSocket( QObject * parent )
: QTcpSocket( parent )
, mIncomingPos( -1 )
, mIncomingSize( -1 )
{
	connect( this, SIGNAL( readyRead() ), SLOT( slotReadyRead() ) );
}

void PacketSocket::sendPacket( const Packet & packet )
{
	QByteArray bytes;
	QDataStream(&bytes, QIODevice::WriteOnly) << packet.id << quint32(packet.data.size());
	bytes.append( packet.data );
	write( bytes );
}

Packet PacketSocket::nextPacket( bool block )
{
	while( block && mReadPackets.isEmpty() ) {
		waitForReadyRead();
		slotReadyRead();
	}
	if( mReadPackets.isEmpty() ) return Packet();
	return mReadPackets.takeFirst();
}

bool PacketSocket::waitForNextPacket( int msec )
{
	if( availablePacketCount() ) return true;
	
	QTime t;
	t.start();
	
	do {
		waitForReadyRead(msec);
		if( bytesAvailable() ) slotReadyRead();
	} while( availablePacketCount() == 0 && isValid() && msec >= 0 && t.elapsed() < msec );

	return availablePacketCount() > 0;
}

void PacketSocket::slotReadyRead()
{
	while( bytesAvailable() > 0 ) {
		if( mIncomingPos == -1 ) {
			if( bytesAvailable() < sizeof(quint32) * 2 )
				return;
			QDataStream(this) >> mIncomingPacket.id >> mIncomingSize;
			mIncomingPos = 0;
			mIncomingPacket.data = QByteArray( mIncomingSize, 0 );
			emit packetStarted( mIncomingPacket.id, mIncomingSize );
		}
		if( bytesAvailable() == 0 ) return;
		
		QByteArray bytes = read( mIncomingSize - mIncomingPos );
		memcpy( mIncomingPacket.data.data() + mIncomingPos, bytes.data(), bytes.size() );
		mIncomingPos += bytes.size();
		emit packetProgress( mIncomingPacket.id, mIncomingSize, mIncomingPos );

		if( mIncomingPos == mIncomingSize ) {
			mReadPackets.append( mIncomingPacket );
			LOG_5( "Full Packet Recieved, Size: " + QString::number( mIncomingPacket.data.size() ) );
			mIncomingPos = mIncomingSize = -1;
			mIncomingPacket = Packet();
			emit packetAvailable();
		}
	}
}

