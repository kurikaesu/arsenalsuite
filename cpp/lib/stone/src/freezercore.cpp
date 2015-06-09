
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

#include <qsqldatabase.h>
#include <qsqlerror.h>

#include "blurqt.h"
#include "database.h"
#include "freezercore.h"
#include "iniconfig.h"

namespace Stone {

ThreadTask::ThreadTask( int type, QObject * reciever )
: QEvent( QEvent::Type(type) )
, mReciever( reciever )
, mCancel( false )
{}

BackgroundThread::BackgroundThread( FreezerCore * fc )
: mExit( false )
, mFC( fc )
, mTask( 0 )
{}

void BackgroundThread::run()
{
	while( !mExit ) {
		// Will wait for a task
		mFC->getTask(this);
		if( !mTask ) continue;
		mTask->run();
		mFC->deliver(this);
		//qApp->wakeUpGuiThread();
	}
}

void ThreadTask::run()
{
}

FreezerCore::FreezerCore()
{
	for( int i=0; i<1; i++ ) {
		BackgroundThread * bt = new BackgroundThread( this );
		mThreads += bt;
		bt->start();
	}
}

FreezerCore::~FreezerCore()
{
	shutdownThreads();
}

void FreezerCore::shutdownThreads()
{
	foreach( BackgroundThread * thread, mThreads ) {
		thread->mExit = true;
	}
	mWait.wakeAll();
	foreach( BackgroundThread * thread, mThreads ) {
		thread->wait();
		delete thread;
	}
}

void FreezerCore::getTask( BackgroundThread * thread )
{
	mTaskMutex.lock();
	if( mTasks.isEmpty() )
		mWait.wait( &mTaskMutex );
	if( !mTasks.isEmpty() ) {
		thread->mTask = mTasks.back();
		mTasks.pop_back();
	}
	mTaskMutex.unlock();
}

void FreezerCore::addTask( ThreadTask * task )
{
	bool needWake = false;
	FreezerCore * fc = instance();
	fc->mTaskMutex.lock();
	needWake = fc->mTasks.isEmpty();
	fc->mTasks.push_front( task );
	fc->mTaskMutex.unlock();
	if( needWake )
		fc->mWait.wakeOne();
}

void FreezerCore::deliver( BackgroundThread * thread )
{
	//mTaskMutex.lock();
	ThreadTask * task = thread->mTask;
	if( task && !task->mCancel && task->mReciever ) {
	//	printf( "Delivering task to object %p\n", task->mReciever );
		QCoreApplication::postEvent( task->mReciever, task );
	} else {
	//	if( task->mReciever && task->mCancel )
	//		printf( "Delivery canceled for task to object %p\n", task->mReciever );
		delete task;
	}
	thread->mTask = 0;
	//mTaskMutex.unlock();
}

void FreezerCore::cancelObjectTasks( QObject * object )
{
	mTaskMutex.lock();
	printf( "Cancelling tasks for object %p\n", object );
	foreach( ThreadTask * task, mTasks ) {
		if( task->mReciever == object ) {
			mTasks.removeAll(task);
			delete task;
		}
	}
	foreach( BackgroundThread * thread, mThreads )
		if( thread->mTask && thread->mTask->mReciever == object )
			thread->mTask->mCancel = true;
	mTaskMutex.unlock();
}

void FreezerCore::cancelTask( ThreadTask * task )
{
	FreezerCore * fc = instance();
	fc->mTaskMutex.lock();
	task->mCancel = true;
	fc->mTasks.removeAll(task);
	fc->mTaskMutex.unlock();
}

class SetThreadDatabaseTask : public ThreadTask
{
public:
	SetThreadDatabaseTask( Database * db, Connection * c )
	: mDb( db ), mConn( c ) {}
	void run() {
		mConn->checkConnection();
		mDb->setConnection( mConn );
		Database::setCurrent( mDb );
	}
	Database * mDb;
	Connection * mConn;
};

void FreezerCore::setDatabaseForThread( Database * db, Connection * c )
{
	addTask( new SetThreadDatabaseTask( db, c ) );
}

void FreezerCore::wakeup()
{
	FreezerCore * fc = instance();
	fc->mWait.wakeOne();
}

void FreezerCore::wait()
{
	mTaskMutex.lock();
	mWait.wait( &mTaskMutex, 50 );
	mTaskMutex.unlock();
}

FreezerCore * FreezerCore::mCore = 0;

FreezerCore * FreezerCore::instance()
{
	if( !mCore ){
		mCore = new FreezerCore;
	}
	return mCore;
}

void FreezerCore::shutdown()
{
	if( mCore ) {
		delete mCore;
		mCore = 0;
	}
}

} //namespace
