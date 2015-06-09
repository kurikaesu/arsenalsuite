
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

#ifndef PACKET_SOCKET_H
#define PACKET_SOCKET_H

#include <qbytearray.h>
#include <qlist.h>
#include <qtcpsocket.h>

#include "blurqt.h"
#include "quickdatastream.h"

struct STONE_EXPORT Packet
{
	Packet( quint32 _id = 0, const QByteArray & _data = QByteArray() ) : id( _id ), data( _data ) {}
	Packet( quint32 _id, const QuickDataStream & qds ) : id( _id ), data( qds.data() ) {}
	bool isValid() const { return id > 0; }
	quint32 id;
	QByteArray data;
};

/**
 *	This class is used facilitate easy and reliable transmission of packets
 *  over a tcp stream.  This is useful for implementing binary protocols, where
 *  you don't need any data until an entire packet is available.  Since packets
 *  may be of arbitrary size, they may be recieved in small chunks over time. This
 *  class gathers the chunks into a full packet then notifies the user via the
 *  packetAvailable signal.
 *
 *  This class is currently designed to operate in buffered mode, turning buffering off
 *  on the socket is likely to result in data loss.
 */
class STONE_EXPORT PacketSocket : public QTcpSocket
{
Q_OBJECT
public:
	PacketSocket( QObject * parent );

	/**
	 *  This function buffers the packet and returns immediatly.
	 *  If the data needs to be sent right away, then the caller should
	 *  call flush on the socket, otherwise the data will be sent
	 *  when control returns to the qt event loop.
	 */
	void sendPacket( const Packet & packet );

	/** 
	 *  Returns the number of packets in the recieve queue.
	 */
	int availablePacketCount() { return mReadPackets.size(); }

	/**
	 *  Pops the next packet from the front of the recieve queue and
	 *  returns it.
	 */
	Packet nextPacket( bool block = false );

	bool waitForNextPacket( int msec );

signals:
	/** Emitted each time a full packet has been read from the socket */
	void packetAvailable();
	
	void packetProgress( quint32 packetType, quint32 packetSize, quint32 progress );

	void packetStarted( quint32 packetType, quint32 packetSize );

protected slots:
	void slotReadyRead();

protected:
	
	qint32 mIncomingPos, mIncomingSize;
	Packet mIncomingPacket;
	QList<Packet> mReadPackets;
};

#endif // PACKET_SOCKET_H

