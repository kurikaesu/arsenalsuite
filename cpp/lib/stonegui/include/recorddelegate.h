
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

#ifndef RECORD_DELEGATE_H
#define RECORD_DELEGATE_H

#include <qitemdelegate.h>

#include "stonegui.h"

class STONEGUI_EXPORT ExtDelegate : public QItemDelegate
{
Q_OBJECT
public:
	ExtDelegate ( QObject * parent = 0 );

	QWidget * createEditor ( QWidget *, const QStyleOptionViewItem &, const QModelIndex & ) const;
	void setModelData ( QWidget *, QAbstractItemModel *, const QModelIndex & ) const;
	void setEditorData ( QWidget *, const QModelIndex & ) const;
	bool editorEvent ( QEvent * event, QAbstractItemModel * model, const QStyleOptionViewItem & option, const QModelIndex & index );
	void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
};

class STONEGUI_EXPORT RecordDelegate : public ExtDelegate
{
Q_OBJECT
public:
	RecordDelegate ( QObject * parent = 0 );

	enum {
		CurrentRecordRole = Qt::UserRole+1,
		FieldNameRole = Qt::UserRole+2
	};

	QWidget * createEditor ( QWidget *, const QStyleOptionViewItem &, const QModelIndex & ) const;
	void setModelData ( QWidget *, QAbstractItemModel *, const QModelIndex & ) const;
	void setEditorData ( QWidget *, const QModelIndex & ) const;
	bool editorEvent ( QEvent * event, QAbstractItemModel * model, const QStyleOptionViewItem & option, const QModelIndex & index );
	void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
};

#endif // RECORD_DELEGATE_H

