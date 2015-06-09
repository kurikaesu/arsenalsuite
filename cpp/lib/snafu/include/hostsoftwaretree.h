
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
 * $Id: hostsoftwaretree.h 6953 2008-07-31 04:13:57Z brobison $
 */

#ifndef HOST_SOFTWARE_TREE_H
#define HOST_SOFTWARE_TREE_H

#include <qtreeview.h>
#include "qvariantcmp.h"


#include "host.h"
#include "software.h"
#include "license.h"
#include "recordtreeview.h"

struct HostSoftwareItem : public RecordItemBase
{
	Record rec;
	Host host;
	License lic;
	Qt::CheckState checked;

	void setup( const Record & r, const QModelIndex & = QModelIndex() ) {
		host = r;
		lic = r;
		rec = r;
		checked = lic.host().isRecord() ? Qt::Checked : Qt::Unchecked;
	}
	Record getRecord() { return rec; }
	QVariant modelData( const QModelIndex & i, int role );
	Qt::ItemFlags modelFlags( const QModelIndex & );
	bool setModelData( const QModelIndex & i, const QVariant & data, int role );
	RecordList children( const QModelIndex & );
	int compare( const QModelIndex & a, const QModelIndex & b, int col, bool asc ) const;
};
typedef TemplateRecordDataTranslator<HostSoftwareItem> HostSoftwareModelBase;

class HostSoftwareModel : public RecordSuperModel
{
Q_OBJECT

public:
	HostSoftwareModel( QObject * parent = 0 );

protected:
	bool dropMimeData ( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent );
};

class HostSoftwareTree : public RecordTreeView
{
Q_OBJECT
public:
	HostSoftwareTree( QWidget * parent = 0 );

	void setRootElement();

protected:
	SoftwareList mSoftwareList;
	LicenseList mLicenseList;
	HostList mHostList;
};

#endif // HOST_SOFTWARE_TREE_H

