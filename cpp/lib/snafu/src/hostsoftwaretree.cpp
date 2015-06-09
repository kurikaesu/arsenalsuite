
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
 * $Id: hostsoftwaretree.cpp 6953 2008-07-31 04:13:57Z brobison $
 */

#include "software.h"
#include "host.h"
#include "license.h"
#include "hostsoftware.h"
#include "hostsoftwaretree.h"
#include "modeliter.h"
#include "recorddrag.h"

QVariant HostSoftwareItem::modelData( const QModelIndex & i, int role ) {
	// all depends on whether it's a Software or License row
	if( host.isRecord() ) {
		if( i.column() == 0 ) {
			if( role == Qt::DisplayRole )
				return host.name();
		}
	} else if ( lic.isRecord() ) {
		if( i.column() == 0 ) {
			if( role == Qt::DisplayRole )
				return lic.software().name() +" "+lic.software().version();
		}
		else if( i.column() == 1 ) {
			if( role == Qt::DisplayRole )
				return lic.license();
		}
	}
	return QVariant();
}
Qt::ItemFlags HostSoftwareItem::modelFlags( const QModelIndex & ) {
	if( host.isRecord() )
		return Qt::ItemFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled );
	else
		return Qt::ItemFlags( Qt::ItemIsEnabled );
}
bool HostSoftwareItem::setModelData( const QModelIndex & i, const QVariant & data, int role ) {
	if( i.column() == 0 && role == Qt::CheckStateRole ) {
		checked = Qt::CheckState(data.toInt());
		return true;
	}
	return false;
}
RecordList HostSoftwareItem::children( const QModelIndex & ) {
	if( host.isRecord() ) {
		LicenseList ll = License::recordsByHost(host);
		return ll;
	}
	return RecordList();
}
int HostSoftwareItem::compare( const QModelIndex & a, const QModelIndex & b, int col, bool asc ) const {
	HostSoftwareItem & other = HostSoftwareModelBase::data(b);
	if( host.isRecord() )
		return compareRetI(host.name(), other.host.name());
	else
		return compareRetI(lic.license(), other.lic.license());
}

HostSoftwareModel::HostSoftwareModel( QObject * parent )
: RecordSuperModel( parent )
{
	QStringList headerLabels;
	headerLabels << "Name";
	headerLabels << "License";
	setHeaderLabels( headerLabels );
	setRootList( Host::select("online=1") );
}

bool HostSoftwareModel::dropMimeData ( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent )
{
	RecordList rl;
	if( parent.isValid() && RecordDrag::decode( data, &rl ) ) {
		Record r = getRecord(parent);
		Host host(r);
		QList<License> added;
		foreach( License lic, rl ) {
			if( added.contains(lic) )
				continue;
			LOG_5( "HostSoftwareTree::dropMimeData: Dropping record: " + r.table()->tableName() + ":" + QString::number( r.key() ) + " on record: " + host.table()->tableName() + ":" + QString::number( host.key() ) );
			lic.setHost(host);
			lic.commit();
			append(lic, parent);
			added += lic;
		}
		return true;
	}
	return RecordSuperModel::dropMimeData(data,action,row,column,parent);
}

HostSoftwareTree::HostSoftwareTree( QWidget * parent )
: RecordTreeView( parent )
{
	setAcceptDrops( true );
	setDropIndicatorShown(true);
	setRootIsDecorated(true);

	RecordSuperModel * hsm = new RecordSuperModel( this );
	new HostSoftwareModel( hsm->treeBuilder() );
	setModel( hsm );
	setSelectionMode( QAbstractItemView::SingleSelection );

	setColumnAutoResize( 0, true );
	mHostList = Host::select("online=1");
}

void HostSoftwareTree::setRootElement()
{
	model()->setRootList( mHostList );
	expandRecursive();
}

