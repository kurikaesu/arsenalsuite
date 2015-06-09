
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
 * $Id: recordlistview.h 13650 2012-10-01 22:12:31Z newellm $
 */

#ifndef RECORD_LIST_VIEW_H
#define RECORD_LIST_VIEW_H

#include <qlistview.h>

#include "recordsupermodel.h"
#include "stonegui.h"

class RecordSuperModel;

class STONEGUI_EXPORT RecordListView : public QListView
{
Q_OBJECT
public:
	RecordListView( QWidget * parent );
	void setModel( RecordSuperModel * model );

	RecordSuperModel * model() const;

	Record current();

	RecordList selection();

public slots:
	void setSelection( const RecordList & rl );
	void setCurrent( const Record & r );

signals:
	void currentChanged( const Record & );
	void selectionChanged( RecordList );

protected slots:
	void slotCurrentChanged( const QModelIndex & i, const QModelIndex & );
	void slotSelectionChanged( const QItemSelection & sel, const QItemSelection & );
};

#endif // RECORD_LIST_VIEW_H

