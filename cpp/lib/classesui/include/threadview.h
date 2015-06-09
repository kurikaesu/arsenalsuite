/*
 *
 * Copyright 2003, 2004 Blur Studio Inc.
 *
 * This file is part of the Resin software package.
 *
 * Resin is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Resin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Resin; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef THREAD_VIEW_H
#define THREAD_VIEW_H

#include "element.h"
#include "job.h"
#include "thread.h"

#include "classesui.h"
#include "ui_threadviewui.h"

class QMenu;
class QPushButton;

class User;
class Element;
class ElementList;
class Job;
class JobList;
class Thread;

class ThreadViewInternal;

class CLASSESUI_EXPORT ThreadView : public QWidget
{
Q_OBJECT
public:
	ThreadView( QWidget * parent=0 );

	void setElementList( ElementList elements );
	void setJobList( JobList jobs );

	ThreadViewInternal * internal() { return mInternal; }

protected:
	ThreadViewInternal * mInternal;
};


class CLASSESUI_EXPORT ThreadModel : public RecordSuperModel
{
Q_OBJECT
public:
	ThreadModel( QObject * parent );

public slots:
	void threadNotifyAddedOrRemoved( RecordList );
	
public:
	bool Thread;
};

class CLASSESUI_EXPORT ThreadViewInternal : public QWidget, public Ui::ThreadViewUI
{
Q_OBJECT
public:
	ThreadViewInternal( QWidget * parent=0 );

	void setElementList( ElementList elements );
	void setJobList( JobList jobs );

public slots:
	void slotAddNote( const Record & record = Element(), const Thread & replyTo = Thread() );

	void slotEditNote( const Thread & );
	
	void showMenu( const QPoint &, const Record &, RecordList );

	void showRecursiveToggled( bool showRecursive );

	void itemSelected( const Record & );
	
	void linkClicked( const QUrl & );
	
	void unreadFilterChanged();
	
	void showLastDaysToggled( bool );
	void showLastDaysChanged( const QString & );

	void readConfig();

protected:
	//bool event( QEvent * );

	int mLastDays;
	bool mShowRecursive;
	bool mIgnoreUpdates;
	bool mUserView;

	ElementList mElements;
	JobList mJobs;
	
	QString mCurrentRT;
};

#endif // THREAD_VIEW_H

