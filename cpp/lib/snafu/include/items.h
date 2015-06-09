
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of Snafu.
 *
 * Snafu is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Snafu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Snafu; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id: items.h 6953 2008-07-31 04:13:57Z brobison $
 */

#ifndef SNAFU_ITEMS_H
#define SNAFU_ITEMS_H

#include <qtreewidget.h>
#include <qdatetime.h>
#include <qitemdelegate.h>

#include "job.h"
#include "jobtype.h"
#include "host.h"
#include "hoststatus.h"
#include "employee.h"
#include "blurqt.h"
#include "recordtreeview.h"
#include "displayprefsdialog.h"
#include "recordsupermodel.h"

#include <math.h>

// color if valid
QVariant civ( const QColor & c );

struct HostItem : public RecordItemBase {
	Host host;
	HostStatus hostStatus;
	QString jobName, ver, mem, mhz, user, pulse;
	ColorOption * co;
	void setup( const Record & r, const QModelIndex & );
	QVariant modelData( const QModelIndex & i, int role ) const;
	char getSortChar() const;
	int compare( const QModelIndex & a, const QModelIndex & b, int column, bool asc );
	Qt::ItemFlags modelFlags( const QModelIndex & );
	Record getRecord();
};

typedef TemplateRecordDataTranslator<HostItem> HostModel;

void setupHostView( RecordTreeView * );
void saveHostView( RecordTreeView * );

class MultiLineDelegate : public QItemDelegate
{
Q_OBJECT
public:
	MultiLineDelegate( QObject * parent=0 ) : QItemDelegate( parent ) {}
	~MultiLineDelegate() {}

	QSize sizeHint(const QStyleOptionViewItem &option,
                              const QModelIndex &index) const;
};

#endif // ITEMS_H

