
/*
 *
 * Copyright 2012 Blur Studio Inc.
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

#ifndef GRAPHITE_PRIVATE_H
#define GRAPHITE_PRIVATE_H

#include <qdatetime.h>
#include <qlist.h>
#include <qmutex.h>
#include <qstring.h>
#include <qthread.h>

class QTcpSocket;

class GraphiteDataPoint;

class GraphiteSender : public QObject
{
Q_OBJECT
public:
	GraphiteSender( const QString & host, quint16 port );
	~GraphiteSender();
	
	
public slots:
	void sendQueuedDataPoints();
	
protected:
	void connectSocket();
	bool event( QEvent * );

	QTcpSocket * mSocket;
	QString mGraphiteHost;
	quint16 mGraphitePort;
};

class GraphiteThread : public QThread
{
Q_OBJECT
public:
	GraphiteThread();
	~GraphiteThread();

	// Thread safe functions
	void record( const QString & path, double value, const QDateTime & timestamp );
	bool hasQueuedRecords();
	GraphiteDataPoint popQueuedRecord();
	
	static GraphiteThread * instance();
	
protected:
	void run();
	static GraphiteThread * mThread;

	QString mGraphiteHost;
	quint16 mGraphitePort;
	GraphiteSender * mSender;
	QMutex mQueueMutex;
	QList<GraphiteDataPoint> mQueue;
	friend class GraphiteSender;
};

#endif // GRAPHITE_PRIVATE_H
