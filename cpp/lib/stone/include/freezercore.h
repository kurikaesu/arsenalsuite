
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

#ifndef FREEZER_CORE_H
#define FREEZER_CORE_H

#include <qmutex.h>
#include <qlist.h>
#include <qthread.h>
#include <qwaitcondition.h>
#include <qevent.h>
#include <qobject.h>
#include <qevent.h>
#include <qsqldatabase.h>

#include "blurqt.h"

namespace Stone {
class BackgroundThread;
class Database;
class Connection;

/**
 *  This class is used as a base class for building objects
 *  that perform work in the background thread.  Objects must
 *  implement the run method, and can return data by storing
 *  it as a member and passing the object back by sending the
 *  object to a qobject as an event
 *  \ingroup Stone
 */

class STONE_EXPORT ThreadTask : public QEvent
{
public:
	ThreadTask( int type = 0, QObject * reciever = 0 );
	
	virtual void run();
	
	bool canceled() const { return mCancel; }
	void cancel() { mCancel = true; }

	QObject * reciever() const { return mReciever; }
	
	QObject * mReciever;
	bool mCancel;
};

/**  This class is responsible for maintaining a database
 *   connection using a QDatabase object.  It retrieves the
 *   connection setting from the config() object.  It also
 *   does locking with a mutex so the connection can be
 *   shared between threads.  It runs worker threads when
 *   there is work to do.
 */
class STONE_EXPORT FreezerCore
: public QObject
{
Q_OBJECT
public:
	FreezerCore();
	~FreezerCore();

	
	/** Returns the single FreezerCore object for each app. */
	static FreezerCore * instance();
	
	/** Shuts down the connection and frees any resources */
	static void shutdown();

	/**  Shuts down the worker threads
	 */
	void shutdownThreads();

	static void addTask( ThreadTask * );
	static void wakeup();
	static void setDatabaseForThread( Database * db, Stone::Connection * c );
	
	// Cancels all tasks that have their reciever set to the object
	void cancelObjectTasks( QObject * );
	
	static void cancelTask( ThreadTask * task );

signals:

protected:
	static FreezerCore * mCore;
	
	// All the threads, including the sql thread
	QList<BackgroundThread*> mThreads;
	QList<ThreadTask*> mTasks;
	QMutex mTaskMutex;
	QWaitCondition mWait;

	// API for background threads to wait for tasks, get tasks, and deliver events after task finishes
	// these methods take care of thread safety issues, including a thread safe mechanism of delivering
	// the event to the QObject( the qobjects that get events must call cancelObjectTasks when they are
	// deleted to ensure there is no race condition).
	void wait();
	void getTask(BackgroundThread * thread);
	void deliver(BackgroundThread * thread);

	friend class BackgroundThread;
};

class STONE_EXPORT BackgroundThread : public QThread
{
public:
	BackgroundThread( FreezerCore * fc );
	
	virtual void run();

protected:
	friend class FreezerCore;
	bool mExit;
	FreezerCore * mFC;
	ThreadTask * mTask;
};

} //namespace

#endif // FREEZER_CORE_H

