
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
 * $Id: softwarelicensetree.cpp 6953 2008-07-31 04:13:57Z brobison $
 */

#include <QMenu>
#include "qvariantcmp.h"

#include "modeliter.h"

#include "software.h"
#include "softwarelicensetree.h"
#include "host.h"
#include "license.h"
#include "hostsoftware.h"
#include "softwaredialog.h"
#include "licensedialog.h"

void SoftwareLicenseItem::setup( const Record & r, const QModelIndex & ) {
	soft = r;
	lic = r;
	rec = r;
	checked = lic.host().isRecord() ? Qt::Checked : Qt::Unchecked;
	//if( soft.isRecord() )
	//	LOG_3("trying to load icon: "+ soft.icon());
	//icon = soft.isRecord() ? QPixmap(soft.icon()) : QPixmap();
}
QVariant SoftwareLicenseItem::modelData( const QModelIndex & i, int role ) {
	// all depends on whether it's a Software or License row
	if( soft.isRecord() ) {
		if( i.column() == 0 ) {
			if( role == Qt::DisplayRole )
				return soft.name() + " " + soft.version();
			if( role == Qt::DecorationRole )
				return icon;
		}
	} else if ( lic.isRecord() ) {
		if( i.column() == 0 ) {
			if( role == Qt::CheckStateRole )
				return checked;
			else if( role == Qt::DisplayRole )
				return lic.license();
		}
		if( i.column() == 1 ) {
			if( role == Qt::DisplayRole )
				return lic.host().name();
		}
	}
	return QVariant();
}
Qt::ItemFlags SoftwareLicenseItem::modelFlags( const QModelIndex & ) {
	if( lic.isRecord() )
		return Qt::ItemFlags( Qt::ItemIsUserCheckable|Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsDragEnabled );
	else
		return Qt::ItemFlags( Qt::ItemIsEnabled|Qt::ItemIsSelectable );
}
bool SoftwareLicenseItem::setModelData( const QModelIndex & i, const QVariant & data, int role ) {
	if( i.column() == 0 && role == Qt::CheckStateRole ) {
		checked = Qt::CheckState(data.toInt());
		return true;
	}
	return false;
}
RecordList SoftwareLicenseItem::children( const QModelIndex & ) {
	if( soft.isRecord() ) {
		LicenseList ll = License::recordsBySoftware(soft);
		return ll;
	}
	return RecordList();
}
int SoftwareLicenseItem::compare( const QModelIndex & a, const QModelIndex & b, int column, bool asc) const {
	SoftwareLicenseItem & other = SoftwareLicenseModel::data(b);
	if( soft.isRecord() )
		return compareRetI(soft.name(), other.soft.name());
	else
		return compareRetI(lic.license(), other.lic.license());
}

SoftwareLicenseTree::SoftwareLicenseTree( QWidget * parent )
: RecordTreeView( parent )
{
	mSoftware = Software::select();

  setDragEnabled( true );
	setRootIsDecorated( true );

	QStringList headerLabels;
	headerLabels << "Name";
	headerLabels << "Host";

        RecordSuperModel * slm = new RecordSuperModel(this);
        new SoftwareLicenseModel( slm->treeBuilder() );
        slm->setAutoSort( true );
        slm->setHeaderLabels( headerLabels );
        slm->setRootList( mSoftware );
        setModel( slm );

	setSelectionMode( QAbstractItemView::SingleSelection );
	setColumnAutoResize( 0, true );

/*
	Table * et = Software::table();
	connect( et, SIGNAL( added( RecordList ) ), SLOT( slotLicenseAdded( RecordList ) ) );
	connect( et, SIGNAL( removed( RecordList ) ), SLOT( slotLicenseRemoved( RecordList ) ) );
	connect( et, SIGNAL( updated( Record, Record ) ), SLOT( slotLicenseUpdated( Record, Record ) ) );
*/

	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(contextMenu(const QPoint &)));

	setIconSize(QSize(24,24));
}

void SoftwareLicenseTree::setRootElement()
{
	model()->setRootList( mSoftware );
	expandRecursive();
}

void SoftwareLicenseTree::contextMenu( const QPoint & point )
{
	QModelIndex mi = indexAt(point);
	Record rec = model()->getRecord(mi);
	QMenu menu;
	if( Software(rec).isRecord() )
		menu.addAction("Add Licenses");
	if( License(rec).isRecord() )
		menu.addAction("Delete License");

	QAction * result = menu.exec( QCursor::pos() );
	if (result)
	{
		if (result->text() == "Add Licenses") {
			LicenseList ll = addLicenses(Software(rec));
			model()->append(ll, mi);
		}
		else if (result->text() == "Delete License")
			deleteLicense(License(rec));
	}
}

void SoftwareLicenseTree::addSoftware()
{
	SoftwareDialog * sd = new SoftwareDialog();
	if( sd->exec() == QDialog::Accepted ) {
		Software soft;
		soft.setName(sd->name());
		soft.setVersion(sd->version());
		soft.commit();
		model()->append(soft);
	}
}

LicenseList SoftwareLicenseTree::addLicenses(const Software & soft)
{
	LicenseList toAdd;
	LicenseDialog * ld = new LicenseDialog();
	if( ld->exec() == QDialog::Accepted ) {
		foreach( QString key, ld->text().split("\n") ) {
			License lic;
			lic.setLicense(key);
			lic.setSoftware(soft);
			lic.setTotal(1);
			toAdd += lic;
		}
		toAdd.commit();
	}
	return toAdd;
}

void SoftwareLicenseTree::deleteLicense(License lic)
{
			model()->remove(lic);
			lic.remove();
			lic.commit();
}

