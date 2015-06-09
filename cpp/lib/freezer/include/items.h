
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of Arsenal.
 *
 * Arsenal is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Arsenal is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Blur; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* $Author: newellm $
 * $LastChangedDate: 2012-06-13 13:42:35 -0700 (Wed, 13 Jun 2012) $
 * $Rev: 13251 $
 * $HeadURL: svn://newellm@svn.blur.com/blur/trunk/cpp/lib/assfreezer/include/items.h $
 */

#ifndef ITEMS_H
#define ITEMS_H

#include <qtreewidget.h>
#include <qdatetime.h>
#include <qitemdelegate.h>
#include <qpainter.h>
#include <qpicture.h>

#include "blurqt.h"

#include "employee.h"
#include "job.h"
#include "jobtask.h"
#include "jobtaskassignment.h"
#include "joberror.h"
#include "jobtype.h"
#include "jobstatus.h"
#include "jobservice.h"
#include "host.h"
#include "hoststatus.h"

#include "hostselector.h"
#include "recordtreeview.h"
#include "recordsupermodel.h"
#include "modelgrouper.h"

#include "afcommon.h"
#include "displayprefsdialog.h"

#include <math.h>

class JobDepList;

// color if valid
QVariant civ( const QColor & c );

struct FREEZER_EXPORT FrameItem : public RecordItemBase
{
	static QDateTime CurTime;
	JobTask task;
	JobTaskAssignment currentAssignment;
	JobOutput output;
	QString hostName, label, time, memory, stat;
	ColorOption * co;
	int loadedStatus;
	void setup( const Record & r, const QModelIndex & );
	QVariant modelData( const QModelIndex & i, int role ) const;
	char getSortChar() const;
	int compare( const QModelIndex & a, const QModelIndex & b, int, bool );
	Qt::ItemFlags modelFlags( const QModelIndex & );
	Record getRecord();
};

typedef TemplateRecordDataTranslator<FrameItem> FrameTranslator;

struct FREEZER_EXPORT JobErrorItem : public RecordItemBase
{
	static const int GroupRole = Qt::UserRole;
	JobError error;
	QString hostName, cnt, when, msg;
	ColorOption * co;
	void setup( const JobError & r, const QModelIndex & );
	QVariant modelData( const QModelIndex & i, int role ) const;
	int compare( const QModelIndex & a, const QModelIndex & b, int, bool );
	Qt::ItemFlags modelFlags( const QModelIndex & ) const;
	Record getRecord();
};

struct FREEZER_EXPORT ErrorItem : public JobErrorItem
{
	Job job;
	JobType jobType;
	QString services;
	void setup( const JobError & r, const QModelIndex & );
	QVariant modelData( const QModelIndex & i, int role ) const;
};

typedef TemplateRecordDataTranslator<JobErrorItem> JobErrorTranslator;
typedef TemplateRecordDataTranslator<ErrorItem> ErrorTranslator;
//typedef RecordModelImp<TreeNodeT<ErrorCache> > ErrorModel;

class FREEZER_EXPORT JobErrorModel : public RecordSuperModel
{
Q_OBJECT
public:
	JobErrorModel( QObject * parent );
};

class FREEZER_EXPORT ErrorModel : public RecordSuperModel
{
Q_OBJECT
public:
	ErrorModel( QObject * parent );

	void setErrors( JobErrorList errors, JobList jobs, JobServiceList services );

	JobList mJobs;
	QMap<Job,JobServiceList> mJobServicesByJob;
};

struct FREEZER_EXPORT JobItem : public RecordItemBase
{
	Job job;
	JobStatus jobStatus;
	QPixmap icon;
	QString done, userName, hostsOnJob, priority, project, submitted, errorCnt, avgTime, type, ended, timeInQueue, services, avgMemory;
	bool healthIsNull;
	ColorOption * co;
	void setup( const Record & r, const QModelIndex & );
	QVariant modelData( const QModelIndex & i, int role ) const;
	int compare( const QModelIndex & a, const QModelIndex & b, int, bool );
	Qt::ItemFlags modelFlags( const QModelIndex & );
	Record getRecord();
	QString userToolTip;
	QString projectToolTip;

	QString efficiency, bytesRead, bytesWrite, diskOps, cpuTime;
};

typedef TemplateRecordDataTranslator<JobItem> JobTranslator;

struct FREEZER_EXPORT GroupedJobItem : public GroupItem
{
	void init( const QModelIndex & idx );
	virtual QString calculateGroupValue( const QModelIndex & self, int column);
	QVariant modelData( const QModelIndex & i, int role ) const;
	Qt::ItemFlags modelFlags( const QModelIndex & );
	QString avgTime, slotsOnGroup;
	ColorOption * colorOption;
};

typedef TemplateDataTranslator<GroupedJobItem> GroupedJobTranslator;

class FREEZER_EXPORT JobTreeBuilder : public RecordTreeBuilder
{
public:
	JobTreeBuilder( SuperModel * parent );
	virtual bool hasChildren( const QModelIndex & parentIndex, SuperModel * model );
	virtual void loadChildren( const QModelIndex & parentIndex, SuperModel * model );
protected:
	JobTranslator * mJobTranslator;
};

class FREEZER_EXPORT JobModel : public RecordSuperModel
{
Q_OBJECT
public:
	JobModel( QObject * parent );

	bool dropMimeData ( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent );

	bool isDependencyTreeEnabled() const { return mDependencyTreeEnabled; }
	void setDependencyTreeEnabled( bool );

	QPixmap jobTypeIcon( const JobType & );
public slots:
	void depsAdded(RecordList);
	void depsRemoved(RecordList);

signals:
	void dependencyAdded( const QModelIndex & parent );
	
protected:
	void addRemoveWorker( JobDepList, bool remove );

	bool mDependencyTreeEnabled;
	QMap<JobType,QPixmap> mJobTypeIconMap;
};

FREEZER_EXPORT void setupJobView( RecordTreeView *, IniConfig & );
FREEZER_EXPORT void saveJobView( RecordTreeView *, IniConfig & );
FREEZER_EXPORT void setupHostView( RecordTreeView *, IniConfig & );
FREEZER_EXPORT void saveHostView( RecordTreeView *, IniConfig & );
FREEZER_EXPORT void setupJobErrorView( RecordTreeView *, IniConfig & );
FREEZER_EXPORT void saveJobErrorView( RecordTreeView *, IniConfig & );
FREEZER_EXPORT void setupHostErrorView( RecordTreeView *, IniConfig & );
FREEZER_EXPORT void saveHostErrorView( RecordTreeView *, IniConfig & );
FREEZER_EXPORT void setupErrorView( RecordTreeView *, IniConfig & );
FREEZER_EXPORT void saveErrorView( RecordTreeView *, IniConfig & );
FREEZER_EXPORT void setupFrameView( RecordTreeView *, IniConfig & );
FREEZER_EXPORT void saveFrameView( RecordTreeView *, IniConfig & );

class FREEZER_EXPORT ProgressDelegate : public QItemDelegate
{
Q_OBJECT
public:
	ProgressDelegate( QObject * parent=0 );
	~ProgressDelegate() {}

	virtual void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
protected:
	ColorOption * mBusyColor, * mNewColor, * mDoneColor, * mSuspendedColor, * mCancelledColor, * mHoldingColor;
	QPixmap taskProgressBar( int, const QString & ) const;
	QPixmap gradientCache( int, const QChar & ) const;
};

class FREEZER_EXPORT LoadedDelegate : public QItemDelegate
{
Q_OBJECT
public:
	LoadedDelegate( QObject * parent=0 ) : QItemDelegate( parent ) {}
	~LoadedDelegate() {}

	virtual void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
};

class FREEZER_EXPORT MultiLineDelegate : public QItemDelegate
{
Q_OBJECT
public:
	MultiLineDelegate( QObject * parent=0 ) : QItemDelegate( parent ) {}
	~MultiLineDelegate() {}

	QSize sizeHint(const QStyleOptionViewItem &option,
                              const QModelIndex &index) const;
};

class FREEZER_EXPORT JobIconDelegate : public QItemDelegate
{
Q_OBJECT
public:
	JobIconDelegate( QObject * parent=0 ) : QItemDelegate( parent ) {}
	~JobIconDelegate() {}
	virtual void paint( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
protected:
	};

#endif // ITEMS_H

