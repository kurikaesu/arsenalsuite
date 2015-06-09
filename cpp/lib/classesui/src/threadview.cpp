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

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qfiledialog.h>
#include <qfileinfo.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qmessagebox.h>
#include <qmime.h>
#include <qpainter.h>
#include <qpoint.h>
#include <qmenu.h>
#include <qpushbutton.h>
#include <qregexp.h>
#include <qstyle.h>
#include <qtextbrowser.h>
#include <qtextedit.h>
#include <qtoolbutton.h>

#include "blurqt.h"
#include "database.h"
#include "freezercore.h"
#include "iniconfig.h"
#include "process.h"

#include "threadview.h"
#include "addnotedialog.h"

#include "element.h"
#include "elementui.h"
#include "employee.h"
#include "task.h"
#include "thread.h"
#include "threadnotify.h"
#include "user.h"
#include "thumbnail.h"
#include "recordlist.h"

static QColor ElementFG, ElementBG, ReadMessageFG, ReadMessageBG, UnreadMessageFG, UnreadMessageBG;

struct ThreadItem : public RecordItemBase
{
	Record parentRecord;
	QString topic, fromUser, dateString, body, userList;
	QColor bg, fg;
	QIcon icon;
	void setup( const Record & rr, const QModelIndex & = QModelIndex() );
	RecordList children( const QModelIndex & i );
	QVariant modelData ( const QModelIndex & i, int role ) const;
	Qt::ItemFlags modelFlags( const QModelIndex &  ) const;
	Record getRecord() const;
	int compare( const QModelIndex & a, const QModelIndex & b, int column, bool asc );
};

typedef TemplateRecordDataTranslator<ThreadItem> ThreadTranslator;

/***********************************************************
 *        Thread View Class
 * 	Provides a simple wrapper widget for ThreadViewInternal
 *
 ***********************************************************/

ThreadView::ThreadView( QWidget * parent )
: QWidget( parent )
{
	QBoxLayout * lay = new QVBoxLayout( this );
	lay->setMargin( 0 );
	mInternal = new ThreadViewInternal( this );
	lay->addWidget( mInternal );
}

void ThreadView::setElementList( ElementList elements )
{
	mInternal->setElementList( elements );
}

void ThreadView::setJobList( JobList jobs )
{
	mInternal->setJobList( jobs );
}

QString breakItUp( const QString & s )
{
	QStringList lines = s.split( "\n" );
	QStringList ret;
	foreach( QString line, lines )
	{
		while( line.length() > 80 ){
			int pos = line.lastIndexOf( ' ', 80 );
			if( pos == -1 )
				pos = 80;
			ret += line.left( pos );
			line = line.mid( pos );
		}
		ret += line;
	}
	return ret.join("\n");
}

void ThreadItem::setup( const Record & rr, const QModelIndex & )
{
	parentRecord = rr;
	Element e(parentRecord);
	Thread t(parentRecord);
	Job j(parentRecord);

	//fg = Qt::black;
	if( e.isRecord() ) {
		topic = e.displayName();
		icon = ElementUi(e).image();
		bg = ElementBG;
		fg = ElementFG;
	} else if( t.isRecord() ) {
		QStringList usl;
		bool unread = false;
		bg = ReadMessageBG;
		fg = ReadMessageFG;
		ThreadNotifyList tnl = ThreadNotify::recordsByThread( t );
		foreach( ThreadNotify tn, tnl )
		{
			if( tn.user() == User::currentUser() && !tn.read() ) 
			{
				bg = UnreadMessageBG;
				fg = UnreadMessageFG;
			}
			else
				usl += tn.user().displayName();
		}

		userList = usl.join("; ");

		topic = t.topic();
		if( topic.isEmpty() )
			topic = t.body().section( QRegExp("\n"), 0, 1 );

		fromUser = t.user().name();
		dateString = t.dateTime().toString();
		body = t.body().replace(QString("\n"), QString(" "));

		if( t.hasAttachments() )
			icon = QIcon( ":/threadview/attach.png" );
	} else if ( j.isRecord() ) {
		topic = j.name();
		//icon = j.jobType().icon();
		bg = ElementBG;
		fg = ElementFG;
    }
}

QVariant ThreadItem::modelData ( const QModelIndex & i, int role ) const
{
	int col = i.column();
	if( role == Qt::DisplayRole ) {
		switch (col) {
			case 0: return topic;
			case 1: return fromUser;
			case 2: return dateString;
			case 3: return body;
			case 4: return userList;
		}
	} else if( role == Qt::BackgroundColorRole )
		return bg;
	else if( role == Qt::TextColorRole )
		return fg;
	else if( role == Qt::DecorationRole && col == 0 )
		return icon;
	return QVariant();
}

Qt::ItemFlags ThreadItem::modelFlags( const QModelIndex &  ) const
{
	return Qt::ItemFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
}

Record ThreadItem::getRecord() const
{ return parentRecord; }

int ThreadItem::compare( const QModelIndex & a, const QModelIndex & b, int column, bool asc )
{
	int i = Thread(parentRecord).isRecord() ? 1 : 0;
	int o = Thread(ThreadTranslator::data(b).parentRecord).isRecord() ? 1 : 0;
	int asci = int(asc);
	if( i != o ) return i - o > 0 ? asci : -asci;
	return RecordItemBase::compare(a,b,column,asc);
}

RecordList ThreadItem::children( const QModelIndex & i )
{
	Element e(parentRecord);
	Job j(parentRecord);
	Thread t(parentRecord);

	RecordList ret;
	if( e.isRecord() ) {
		if( ((ThreadModel*)i.model())->Thread )
			ret += e.children();
		ret += Thread::recordsByElement(e);
	} else if ( j.isRecord() ) {
        ret += Thread::recordsByJob(j);
	} else if ( t.isRecord() ) {
        ret += Thread::select("skeyreply=?", VarList() << t.key());
    }
	return ret;
}

ThreadModel::ThreadModel( QObject * parent )
: RecordSuperModel( parent )
, Thread( false )
{
	new ThreadTranslator(treeBuilder());
	listen( Element::table() );
	listen( Thread::table() );
	connect( ThreadNotify::table(), SIGNAL( added( RecordList ) ), SLOT( threadNotifyAddedOrRemoved( RecordList ) ) );
	connect( ThreadNotify::table(), SIGNAL( removed( RecordList ) ), SLOT( threadNotifyAddedOrRemoved( RecordList ) ) );
}

void ThreadModel::threadNotifyAddedOrRemoved( RecordList recs )
{
	ThreadNotifyList tnl(recs);
	ThreadList tl = tnl.threads().unique();
	foreach( ::Thread t, tl )
		updated( t );
}

/***********************************************************
 *        ThreadListViewInternal
 * This is the UI-inherited widget that handles all the data
 * Provides filtering of threads by the selected elements
 * and by the last days combobox
 *
 ***********************************************************/

/*
enum {
	ELEMENT_NOTE = QEvent::User,
	USER_NOTE,
    JOB_NOTE
};
*/

ThreadViewInternal::ThreadViewInternal( QWidget * parent )
: QWidget( parent )
, mLastDays( 0 )
, mShowRecursive( true )
, mIgnoreUpdates( false )
{
	setupUi( this );

	connect( mAddNoteButton, SIGNAL( clicked() ), SLOT( slotAddNote() ) );
	//connect( mShowRecursiveButton, SIGNAL( toggled( bool ) ), SLOT( showRecursiveToggled( bool ) ) );

	connect( Element::table(), SIGNAL( added( RecordList ) ), SLOT( elementsAdded( RecordList ) ) );
	connect( Element::table(), SIGNAL( removed( RecordList ) ), SLOT( elementsRemoved( RecordList ) ) );

	connect( Thread::table(), SIGNAL( added( RecordList ) ), SLOT( threadsAddedOrRemoved( RecordList ) ) );
	connect( Thread::table(), SIGNAL( removed( RecordList ) ), SLOT( threadsAddedOrRemoved( RecordList ) ) );
	connect( Thread::table(), SIGNAL( update( const Record &, const Record & ) ), SLOT( threadUpdated( const Record &, const Record & ) ) );

	connect( mMessageLabel, SIGNAL( anchorClicked( const QUrl & ) ), SLOT( linkClicked( const QUrl & ) ) );

	//connect( mUnreadFilter, SIGNAL( activated( int ) ), SLOT( unreadFilterChanged() ) );

	//connect( mShowLastCheck, SIGNAL( toggled( bool ) ), SLOT( showLastDaysToggled( bool ) ) );

	//connect( mDaysCombo, SIGNAL( activated( const QString & ) ), SLOT( showLastDaysChanged( const QString & ) ) );

	readConfig();

	ThreadModel * tm = new ThreadModel( mThreadView );
	tm->setHeaderLabels( QStringList() << "Subject" << "From" << "Date" << "Body" << "To" );
	mThreadView->setModel( tm );
	mThreadView->setColumnAutoResize( 0, true );

	connect( mThreadView, SIGNAL( showMenu( const QPoint &, const Record &, RecordList ) ), SLOT( showMenu( const QPoint &, const Record &, RecordList ) ) );
	connect( mThreadView, SIGNAL( currentChanged( const Record & ) ), SLOT( itemSelected( const Record & ) ) );
}

void ThreadViewInternal::readConfig()
{
	IniConfig & cfg = userConfig();
	cfg.pushSection( "Note View Prefs" );
	ElementFG = cfg.readColor( "Element FG" );
	ElementBG = cfg.readColor( "Element BG", QColor( 220, 230, 240 ) );
	ReadMessageFG = cfg.readColor( "Read Message FG" );
	ReadMessageBG = cfg.readColor( "Read Message BG", QColor( 248, 245, 190 ) );
	UnreadMessageFG = cfg.readColor( "Unread Message FG" );
	UnreadMessageBG = cfg.readColor( "Unread Message BG", QColor( 248, 145, 190 ) );
	mThreadView->setFont( cfg.readFont( "Font", mThreadView->font() ) );
	cfg.popSection();
}

static ThreadList filterUnread( User u, ThreadList tl )
{
	ThreadList ret;
	foreach( Thread t, tl )
	{
		ThreadNotifyList tnl = ThreadNotify::recordsByThread( t );
		foreach( ThreadNotify tn, tnl )
			if( tn.user() == u && !tn.read() )
				ret += t;
	}
	return ret;
}

/*
bool ThreadViewInternal::event( QEvent * evt )
{
	bool runDelAndSort = false;
	if( evt->type() == (QEvent::Type)ELEMENT_NOTE ) {
		ElementNoteTask * ent = (ElementNoteTask*)evt;
		for( QMap<Element,ThreadList>::Iterator it = ent->mElementThreads.begin(); it != ent->mElementThreads.end(); ++it )
		{
			ThreadList tl = it.value();
			
			if( mUnreadFilter->currentIndex() == 1 )
				tl = filterUnread( User::currentUser(), tl );
		}
		runDelAndSort = true;
	}
	if( evt->type() == (QEvent::Type)USER_NOTE ) {
		UserNoteTask * unt = (UserNoteTask*)evt;
		runDelAndSort = true;
	}
	if( evt->type() == (QEvent::Type)JOB_NOTE ) {
		JobNoteTask * jnt = (JobNoteTask*)evt;
		runDelAndSort = true;
	}
	if( runDelAndSort ) {
		return true;
	}
	return QWidget::event( evt );
}
*/

void ThreadViewInternal::unreadFilterChanged()
{
	setElementList( mElements );
}

void ThreadViewInternal::showLastDaysToggled( bool sld )
{
	Q_UNUSED(sld);
	int ld = mLastDays;
	//mDaysCombo->setEnabled( sld );
	//mLastDays = sld ? mDaysCombo->currentText().toInt() : 0;
	setElementList( mElements );
}

void ThreadViewInternal::showLastDaysChanged( const QString & ldt )
{
	Q_UNUSED(ldt);
	int ld = mLastDays;
	//mLastDays = mShowLastCheck->isChecked() ? ldt.toInt() : 0;
	//if( ld > mLastDays )
		//deleteOlder( (ThreadListViewItem*)(mListView->firstChild()), mLastDays );
	//else if( ld != mLastDays )
		setElementList( mElements );
}

void ThreadViewInternal::setElementList( ElementList elements )
{
	mMessageLabel->setPlainText("");
	mElements = elements;

	mUserView = UserList(elements).size();
	//mShowRecursiveButton->setEnabled( !mUserView );
	//mAddNoteButton->setEnabled( !mUserView );
	//mShowRecursive = mUserView ? false : mShowRecursiveButton->isChecked();

	mThreadView->model()->setRootList( mElements );
	mThreadView->expandRecursive();
}

void ThreadViewInternal::setJobList( JobList jobs )
{
	mMessageLabel->setPlainText("");
    mJobs = jobs;

    // Update the threads for the jobs selected to ensure any new threads get displayed.
    if( jobs.size() ) {
        QList<unsigned int> keys = jobs.keys();

        QStringList temp;
        VarList vars;
        for( unsigned int i=0; i<keys.size(); ++i ) {
            vars << keys[i];
            temp << "'?'";
        }

        ThreadList tl = Thread::select("fkeyjob in (" + temp.join(",") + ")", vars);
    }

	mThreadView->model()->setRootList( mJobs );
	mThreadView->expandRecursive();
}

void ThreadViewInternal::itemSelected( const Record & r )
{
	Element e(r);
	Thread t(r);

	QString rt;
	if( t.isRecord() )
	{
		rt += "<table cellspacing=1 border=1 width=100% bgcolor=#6688BB><tr><td valign=\"top\"><font size=\"+1\">";
		rt += t.topic();
		rt += "</font><br><b>From:</b> ";
		rt += t.user().displayName();
		rt += "<br><b>To:</b> ";
		
		UserList to = ThreadNotify::recordsByThread( t ).users();
		QStringList usl;
		foreach( User u, to )
			if( u.isRecord() )
				usl += u.displayName();

		rt += usl.join("; ");
		rt += "</td></tr></table><br>";
		rt += t.body().replace("\n","<br>");
		QString attachPath( t.attachmentsPath() );
		QStringList urls = t.attachmentFiles();
		if( !urls.isEmpty() ){
			t.setHasAttachments( true );
			t.commit();
			rt += "<br><font size=\"+1\">Attachments</font><br>";
			for( QStringList::Iterator it = urls.begin(); it != urls.end(); ++it ){
				rt += "<img src=\"icon_" + *it + "\">";
				//QMimeSourceFactory::defaultFactory()->setImage( "icon_" + *it, iconForFile( attachPath + *it ).convertToImage() );
				rt += "<a href=\"" + attachPath + *it + "\">" + *it + "</a><br>";
				QImage img( attachPath + *it );
				if( !img.isNull() ) {
					rt += "<img src=\"" + *it + "\"><br>";
					//QMimeSourceFactory::defaultFactory()->setImage( *it, img );
				}
			}
		}
	}
	mCurrentRT = rt;
	mMessageLabel->setHtml( rt );
	ThreadNotifyList tnl = ThreadNotify::recordsByThread( t );
	foreach( ThreadNotify tn, tnl ) {
		if( tn.user() == User::currentUser() ){
			tn.setRead( true );
			tn.commit();
		}
	}
}

void ThreadViewInternal::linkClicked( const QUrl & loc )
{
	openURL( loc.toString() );
	mMessageLabel->setPlainText( mCurrentRT );
}

void ThreadViewInternal::slotEditNote( const Thread & t )
{
	if( !t.isRecord() )
		return;

	AddNoteDialog * d = new AddNoteDialog( this );
	d->setThread( t );
	d->exec();
	delete d;
}

void ThreadViewInternal::slotAddNote( const Record & record, const Thread & replyTo )
{
	Element el( record );
	if( !el.isRecord() && !mElements.isEmpty() )
		el = mElements[0];

    Job j( record );
    if( !j.isRecord() && !mJobs.isEmpty() )
        j = mJobs[0];

	AddNoteDialog * d = new AddNoteDialog( this );
	d->setElement( el );
	//d->setJob( j );
    d->setJobs( mJobs );
	d->setReplyTo( replyTo );
	d->exec();
	delete d;
}

void ThreadViewInternal::showRecursiveToggled( bool showRecursive )
{
	if( showRecursive == mShowRecursive ) return;
	mShowRecursive = showRecursive;
	((ThreadModel*)mThreadView->model())->Thread = mShowRecursive;
	mAddNoteButton->setEnabled( !showRecursive );
	setElementList( mElements );
}

void ThreadViewInternal::showMenu( const QPoint & point, const Record & cr, RecordList rl )
{
	LOG_3( "ThreadViewInternal::showMenu" );
	Record r(cr);
	QMenu * p = new QMenu( this );
	QAction * newNote = 0, * edit = 0, * reply = 0, * del = 0;

	// If they have one selected, but clicked in an empty space
	if( !r.isRecord() && !rl.isEmpty() )
		r = rl[0];

	Thread t(r);
	Element e(r);
	Job j(r);

	if( r.isRecord() ) {
		Record r;
		newNote = p->addAction( "New Note to " + (t.isRecord() ? t.element().displayName() : e.displayName()) );
	}
	else if( mElements.size() == 1 && !mShowRecursive && !mUserView )
		newNote = p->addAction("New Note" );
	
	if( t.isRecord() ){
		edit = p->addAction( "Edit" );
		//User::permAction( edit, t.user() == User::currentUser() || User::hasPerms( "Thread", true ) );
		
		reply = p->addAction("Reply");
		p->addSeparator();
		del = p->addAction("Delete");
		User::permAction( del, "Thread", true );
	}
	QAction * res = p->exec( point );
	if( !res ) {
		delete p;
		return;
	}
	if( res == newNote ) {
		slotAddNote( r );
	} else if( res == reply ) {
		slotAddNote( r, t );
	} else if( res == del ) {
		ThreadList to_del(rl);
		Database::current()->beginTransaction( "Delete Note" );
		to_del.remove();
		Database::current()->commitTransaction();
	} else if( res == edit ) {
		slotEditNote( t );
	}
	delete p;
}

