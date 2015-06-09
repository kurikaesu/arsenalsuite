
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
 * $Id: softwarelicensetree.h 6953 2008-07-31 04:13:57Z brobison $
 */

#ifndef SOFTWARE_LICENSE_TREE_H
#define SOFTWARE_LICENSE_TREE_H

#include <qtreeview.h>

#include "host.h"
#include "software.h"
#include "license.h"
#include "recordtreeview.h"

struct SoftwareLicenseItem : public RecordItemBase
{
	Record rec;
	Software soft;
	License lic;
	Qt::CheckState checked;
	QPixmap icon;

	void setup( const Record & r, const QModelIndex & = QModelIndex() );
	Record getRecord() { return rec; }
	QVariant modelData( const QModelIndex & i, int role );
	Qt::ItemFlags modelFlags( const QModelIndex & );
	bool setModelData( const QModelIndex & i, const QVariant & data, int role );
	RecordList children( const QModelIndex & );
	int compare( const QModelIndex &, const QModelIndex &, int column, bool asc) const;
};

typedef TemplateRecordDataTranslator<SoftwareLicenseItem> SoftwareLicenseModel;

class SoftwareLicenseTree : public RecordTreeView
{
Q_OBJECT
public:
	SoftwareLicenseTree( QWidget * parent = 0 );

	void setRootElement();

public slots:
	void contextMenu(const QPoint &);
	void addSoftware();
	LicenseList addLicenses( const Software & );
	void deleteLicense( License );

protected:
	SoftwareList mSoftware;
	LicenseList mLicense;
};

#endif // SOFTWARE_LICENSE_TREE_H

