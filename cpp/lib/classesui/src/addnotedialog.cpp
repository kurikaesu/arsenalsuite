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
#include <qgroupbox.h>
#include <qlineedit.h>
#include <qlistwidget.h>
#include <qmenu.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>
#include <qtextedit.h>
#include <qmessagebox.h>
#include <QImageReader>
#include <QImageWriter>
#include <QFileIconProvider>

#include "database.h"
#include "threadnotify.h"
#include "blurqt.h"
#include "addnotedialog.h"
#include "path.h"
#include "recordsupermodel.h"
#include "elementuser.h"
#include "elementui.h"
#include "project.h"

ElementAction::ElementAction( const Element & e, QWidget * parent )
: QAction( parent )
, element( e )
{
	setText( e.displayName() );
	QPixmap p = ElementUi(e).image();
	if( !p.isNull() )
		setIcon( QIcon(p) );
}

/***********************************************************
 *		AddNoteDialog
 *	This is a dialog that allows the user to enter
 *	a subject, message, and note.  It also allow them
 *	to specify what users should be notified about the
 *	note
************************************************************/

struct UserItem : public RecordItemBase
{
	User user;
	QString display;
	void setup( const Record & r, const QModelIndex & ) {
		user = r;
		display = user.displayName();
	}
	QVariant modelData( const QModelIndex & i, int role ) const {
		return ( i.column() == 0 && role == Qt::DisplayRole ) ? display : QVariant();
	}
	Record getRecord() { return user; }
};

typedef TemplateRecordDataTranslator<UserItem> UserTranslator;

AddNoteDialog::AddNoteDialog( QWidget * parent )
: QDialog( parent )
{
	setupUi( this );
}

void AddNoteDialog::setThread( const Thread & thread )
{
	mThread = thread;
	setSubject( mThread.topic() );
	setJobs( ThreadList(mThread.job()) );
	setBody( mThread.body() );
    mReplyThread = mThread.reply();
}

Thread AddNoteDialog::thread() const
{
	return mThread;
}

void AddNoteDialog::setSubject( const QString & subject )
{
	mSubject->setText( subject );
}

void AddNoteDialog::setBody( const QString & body )
{
	mBody->setPlainText( body );
}

void AddNoteDialog::setJobs( const JobList & jobs )
{
    mJobs = jobs;
}

void AddNoteDialog::setReplyTo( const Thread & thread )
{
	mReplyThread = thread;
	if( thread.isRecord() ) {
		QString newSubject = thread.topic();
		if( !newSubject.contains( "RE:" ) )
			newSubject = "RE: " + newSubject;
		setSubject( newSubject );
	}
}

void AddNoteDialog::accept()
{
    foreach( Job mJob, mJobs){
        Thread thread = mThread.copy();
		
    	thread.setJob( mJob );
    	thread.setBody( mBody->toPlainText() );
    	thread.setTopic( mSubject->text() );
    	thread.setReply( mReplyThread );
    	thread.setUser( User::currentUser() );

    	thread.commit();
        mThread = thread;
    }
	QDialog::accept();
}


