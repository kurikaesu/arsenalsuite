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

#ifndef ADD_NOTE_DIALOG_H
#define ADD_NOTE_DIALOG_H

#include "classesui.h"

#include "ui_addnotedialogui.h"
#include "user.h"
#include "thread.h"
#include "job.h"

class kpMainWindow;
class QListWidgetItem;

class CLASSESUI_EXPORT ElementAction : public QAction
{
Q_OBJECT
public:
	Element element;
	ElementAction( const Element & e, QWidget * parent = 0 );
};

class CLASSESUI_EXPORT AddNoteDialog : public QDialog, public Ui::AddNoteDialogUI
{
 Q_OBJECT
 public:
 	AddNoteDialog( QWidget * parent=0 );
	
	void setThread( const Thread & );
	Thread thread() const;

	void setElement( const Element & );
	void setJobs( const JobList & );
	void setReplyTo( const Thread & );

	void setSubject( const QString & subject );
	void setBody( const QString & body );

	UserList notifyList() const;
	void setNotifyList( const UserList & ul );

	void setRelated( UserList supers, UserList coords, UserList producs, UserList assigned );
	void refreshButtons();
	void refresh();
	UserList usersToNotify() const;
	bool requireSignoff() const;
	QStringList attachmentURLs() const;

	void addAttachment( const QString & );

	virtual void accept();

public slots:
	void appendList( UserList list );
	void slotNotifyNew();
	void slotNotifyDelete();
	void slotNotifyClear();

	void slotAddSupervisors();	
	void slotAddCoordinators();
	void slotAddProducers();	
	void slotAddAssigned();

	void slotAddUser( QAction * );

	void slotAddAttachment();
	void slotAttachmentPopup( QListWidgetItem *, const QPoint & );

protected:
	Element mElement;
	JobList mJobs;
	Thread mThread;
	Thread mReplyThread;

	QMap<QString,QString> mAttachmentURLs;
	QMap<QString,QPixmap> mModifiedImages;

	UserList mUserList, mSupers, mCoords, mProducs, mAssigned;
	QMenu *mSupMenu, *mCoordMenu, *mProdMenu, *mAssMenu;
};

#endif // ADD_NOTE_DIALOG_H

