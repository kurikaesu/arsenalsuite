
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
 * along with Arsenal; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id$
 */

#ifndef SERVICE_CHECK_TREE_H
#define SERVICE_CHECK_TREE_H

#include <qtreeview.h>

#include "host.h"
#include "service.h"
#include "recordtreeview.h"

struct ServiceCheckItem : public RecordItemBase
{
	Service e;
	Qt::CheckState checked;
	void setup( const Record & r, const QModelIndex & = QModelIndex() );
	Record getRecord() { return e; }
	QVariant modelData( const QModelIndex & i, int role );
	Qt::ItemFlags modelFlags( const QModelIndex & );
	bool setModelData( const QModelIndex & i, const QVariant & data, int role );
	int compare( const QModelIndex & a, const QModelIndex & b, int column, bool asc) const;
};

typedef TemplateRecordDataTranslator<ServiceCheckItem> ServiceCheckModel;

class ServiceCheckTree : public RecordTreeView
{
Q_OBJECT
public:
	ServiceCheckTree( QWidget * parent = 0 );

	void setRootElement();
	ServiceList checkedElements();
	ServiceList noChangeElements();

	void setChecked( ServiceList els );
	void setNoChange( ServiceList els );

	void setReadOnly( bool );
	
protected:
	ServiceList elementsByState( Qt::CheckState );

	bool mReadOnly;
	ServiceList mService;
};

#endif // SOFTWARE_CHECK_TREE_H

