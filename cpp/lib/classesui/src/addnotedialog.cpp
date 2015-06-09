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
#include "usertaskdialog.h"
#include "recordsupermodel.h"
//#include "kpmainwindow.h"
//#include "kpdocument.h"
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

	connect( mNotifyNew, SIGNAL( clicked() ), SLOT( slotNotifyNew() ) );
	connect( mNotifyDelete, SIGNAL( clicked() ), SLOT( slotNotifyDelete() ) );
	connect( mNotifyClear, SIGNAL( clicked() ), SLOT( slotNotifyClear() ) );
	
	connect( mNotifySupervisor, SIGNAL( clicked() ), SLOT( slotAddSupervisors() ) );
	connect( mNotifyCoordinator, SIGNAL( clicked() ), SLOT( slotAddCoordinators() ) );
	connect( mNotifyProducer, SIGNAL( clicked() ), SLOT( slotAddProducers() ) );
	connect( mNotifyAssigned, SIGNAL( clicked() ), SLOT( slotAddAssigned() ) );
	
	mSupMenu = new QMenu( this );
	mCoordMenu = new QMenu( this );
	mProdMenu = new QMenu( this );
	mAssMenu = new QMenu( this );
	
	mNotifySupervisor->setMenu( mSupMenu );
	mNotifyCoordinator->setMenu( mCoordMenu );
	mNotifyProducer->setMenu( mProdMenu );
	mNotifyAssigned->setMenu( mAssMenu );
	
	mTodoStatusCombo->addItem( "None" );
	mTodoStatusCombo->addItem( "Unresolved" );
	mTodoStatusCombo->addItem( "Resolved" );
	
	connect( mSupMenu, SIGNAL( triggered(QAction*) ), SLOT( slotAddUser(QAction*) ) );
	connect( mCoordMenu, SIGNAL( triggered(QAction*) ), SLOT( slotAddUser(QAction*) ) );
	connect( mProdMenu, SIGNAL( triggered(QAction*) ), SLOT( slotAddUser(QAction*) ) );
	connect( mAssMenu, SIGNAL( triggered(QAction*) ), SLOT( slotAddUser(QAction*) ) );
	
	connect( mAddAttachment, SIGNAL( clicked() ), SLOT( slotAddAttachment() ) );
	
	RecordSuperModel * model = new RecordSuperModel( mNotifyList );
	model->setAutoSort( true );
	new UserTranslator(model->treeBuilder());

	mNotifyList->setModel( model );
	connect( mAttachmentList, SIGNAL( contextMenuRequested( QListWidgetItem*, const QPoint & ) ),
		SLOT( slotAttachmentPopup( QListWidgetItem*, const QPoint & ) ) );

	refresh();
	refreshButtons();
}

void AddNoteDialog::setThread( const Thread & thread )
{
	mThread = thread;
	setSubject( mThread.topic() );
	setElement( mThread.element() );
	setJobs( ThreadList(mThread.job()) );
	setBody( mThread.body() );
	appendList( mThread.threadNotifies().users() );
    if( mThread.isRecord() )
        mAttachmentList->addItems( mThread.attachmentFiles() );
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

UserList AddNoteDialog::notifyList() const
{
	return mUserList;
}

void AddNoteDialog::setNotifyList( const UserList & ul )
{
	mUserList = ul;
	refresh();
}

void addItems( QMenu * menu, UserList users )
{
	foreach( User u, users )
		if( u.isRecord() )
			menu->addAction( new ElementAction( u, menu ) );
}

void AddNoteDialog::setRelated( UserList supers, UserList coords, UserList producs, UserList assigned )
{
	mSupers = supers;
	mSupMenu->clear();
	addItems( mSupMenu, supers );
	mSupMenu->addSeparator();
	mSupMenu->addAction( "Add All", this, SLOT( slotAddSupervisors() ) );
	mCoords = coords;
	mCoordMenu->clear();
	addItems( mCoordMenu, coords );
	mCoordMenu->addSeparator();
	mCoordMenu->addAction( "Add All", this, SLOT( slotAddCoordinators() ) );
	mProducs = producs;
	mProdMenu->clear();
	addItems( mProdMenu, producs );
	mProdMenu->addSeparator();
	mProdMenu->addAction( "Add All", this, SLOT( slotAddProducers() ) );
	mAssigned = assigned;
	mAssMenu->clear();
	addItems( mAssMenu, assigned );
	mAssMenu->addSeparator();
	mAssMenu->addAction( "Add All", this, SLOT( slotAddAssigned() ) );
	refreshButtons();
}

void AddNoteDialog::refreshButtons()
{
	mNotifySupervisor->setEnabled( !mSupers.isEmpty() );
	mNotifyCoordinator->setEnabled( !mCoords.isEmpty() );
	mNotifyProducer->setEnabled( !mProducs.isEmpty() );
	mNotifyAssigned->setEnabled( !mAssigned.isEmpty() );
}

void AddNoteDialog::refresh()
{
	mNotifyList->model()->setRootList( mUserList );
	bool empty = mUserList.isEmpty();
	mNotifyDelete->setEnabled( !empty );
	mNotifyClear->setEnabled( !empty );
	refreshButtons();
}

QStringList AddNoteDialog::attachmentURLs() const
{
	QStringList ret;
	for( QMap<QString,QString>::ConstIterator it = mAttachmentURLs.begin(); it != mAttachmentURLs.end(); ++it )
		ret += *it;
	return ret;
}

void AddNoteDialog::appendList( UserList list )
{
	mUserList |= list;
	refresh();
}

UserList AddNoteDialog::usersToNotify() const
{
	return mUserList;
}

bool AddNoteDialog::requireSignoff() const
{
	return mSignoffCheck->isChecked();
}

void AddNoteDialog::addAttachment( const QString & url )
{
	if( !url.isEmpty() ){
		QString fn = QFileInfo( url ).fileName();
		mAttachmentURLs[fn] = url;
		QListWidgetItem * it = new QListWidgetItem( fn, mAttachmentList );
		it->setIcon( QFileIconProvider().icon( url ) );
	}
}

void AddNoteDialog::slotAddAttachment()
{
    QString openPath = mAttachmentURLs.isEmpty() ? QDir::homePath() : Stone::Path( mAttachmentURLs[0] ).dirPath();
    QStringList selectedFiles = QFileDialog::getOpenFileNames( this, "Choose file(s) to attach", openPath );
    foreach( QString file, selectedFiles ) {
        addAttachment( file );
    }
}

void AddNoteDialog::slotAttachmentPopup( QListWidgetItem * item, const QPoint & pos )
{
	if( !item || !mAttachmentURLs.contains( item->text() ))
		return;

	QMenu * menu = new QMenu( this );
	QAction * del = menu->addAction( "Delete Attachment" );
	QAction * edit = 0;

	QAction * res = menu->exec( pos );
	if( res && res == del )
	{
		mAttachmentURLs.remove( item->text() );
		delete item;
	}
	delete menu;
}

void AddNoteDialog::slotNotifyNew()
{
	UserTaskDialog * utd = new UserTaskDialog( this );
	utd->show();
	if( utd->exec() == QDialog::Accepted )
		appendList( utd->userList() );
	delete utd;
}

void AddNoteDialog::slotNotifyDelete()
{
	mUserList.remove( mNotifyList->current() );
	refresh();
}

void AddNoteDialog::slotNotifyClear()
{
	mUserList.clear();
	refresh();
}

void AddNoteDialog::slotAddSupervisors()
{
	appendList( mSupers );
}

void AddNoteDialog::slotAddCoordinators()
{
	appendList( mCoords );
}

void AddNoteDialog::slotAddProducers()
{
	appendList( mProducs );
}

void AddNoteDialog::slotAddAssigned()
{
	appendList( mAssigned );
}

void AddNoteDialog::slotAddUser( QAction * a )
{
	if( a->inherits( "ElementAction" ) )
		appendList( ((ElementAction*)a)->element );
}

void AddNoteDialog::setElement( const Element & element )
{
	mElement = element;
	UserList assigned;
	ElementUserList tul = ElementUser::recordsByElement( element );
	foreach( ElementUser eu, tul ) {
		User u = eu.user();
		if( u.isRecord() )
			assigned += u;
	}
	setRelated( element.supervisors(), element.coordinators(), element.producers(), assigned );
	setNotifyList( assigned );
}

void AddNoteDialog::setJobs( const JobList & jobs )
{
    mJobs = jobs;
}

void AddNoteDialog::setReplyTo( const Thread & thread )
{
	mReplyThread = thread;
	if( thread.isRecord() ) {
		if( thread.element().isRecord() )
			setElement( thread.element() );
		QString newSubject = thread.topic();
		if( !newSubject.contains( "RE:" ) )
			newSubject = "RE: " + newSubject;
		setSubject( newSubject );
		mUserList -= User::currentUser();
		mUserList |= thread.user();
		refresh();
	}
}

void AddNoteDialog::accept()
{
	QStringList aurls = attachmentURLs();

    foreach( Job mJob, mJobs){
        Thread thread = mThread.copy();

    	thread.setElement( mElement );
    	thread.setJob( mJob );
    	thread.setBody( mBody->toPlainText() );
    	thread.setTopic( mSubject->text() );
    	thread.setReply( mReplyThread );
    	thread.setUser( User::currentUser() );
    	thread.setTodoStatus( mTodoStatusCombo->currentIndex() );
    	thread.setDateTime( QDateTime::currentDateTime() );
    	thread.setHasAttachments( !aurls.isEmpty() );

    	Database::current()->beginTransaction( thread.isRecord() ? "Edit Note" : "Add Note" );
    	thread.commit();

    	// List of users to notify
    	UserList notifyList = usersToNotify();
    	ThreadNotifyList existing = ThreadNotify::recordsByThread( thread );

    	// Remove the ones that already have thread notify records
    	notifyList -= existing.users();
	
    	ThreadNotify tn;
    	tn.setThread( thread );
    	tn.setRequiresSignoff( requireSignoff() );
    	foreach( User u, notifyList )
    	{
    		ThreadNotify cp = tn.copy();
    		cp.setUser( u );
    		cp.commit();
    	}

    	// Delete the ones that have been removed
    	foreach( ThreadNotify tn, existing ) {
    		if( !notifyList.contains( tn.user() ) )
    			tn.remove();
    	}

    	Database::current()->commitTransaction();

    	if( !aurls.isEmpty() ) {
    		QString attachPath( thread.attachmentsPath() );
    		if( attachPath.isEmpty() ) {
    			thread.setHasAttachments( false );
    			thread.commit();
    			QMessageBox::critical( this, "Unable to copy attachments",
    				"The attachments could not be copied. Please notify IT." );
    		} else {
    			foreach( QString url, aurls )
    			{
    				if( !Stone::Path::copy( url, attachPath + QFileInfo(url).fileName() ) ){
    					QMessageBox::critical( this, "Unable to copy attachment",
    						"The attachment '" + url + "' could not be copied. Please notify IT." );
    				}
    			}
    		}
    	}
        mThread = thread;
    }
	QDialog::accept();
}


