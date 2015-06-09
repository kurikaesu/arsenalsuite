
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
 * $Id: servicechecktree.cpp 6953 2008-07-31 04:13:57Z brobison $
 */

#include "qvariantcmp.h"

#include "service.h"
#include "servicechecktree.h"
#include "host.h"
#include "hostservice.h"
#include "modeliter.h"

void ServiceCheckItem::setup( const Record & r, const QModelIndex & ) {
	e = r;
	checked = Qt::Unchecked;
}

QVariant ServiceCheckItem::modelData( const QModelIndex & i, int role ) {
	if( i.column() == 0 ) {
		if( role == Qt::CheckStateRole )
			return checked;
		else if( role == Qt::DisplayRole )
			return e.service();
	}
	return QVariant();
}
Qt::ItemFlags ServiceCheckItem::modelFlags( const QModelIndex & ) {
	return Qt::ItemFlags( Qt::ItemIsUserCheckable | Qt::ItemIsEnabled );
}

bool ServiceCheckItem::setModelData( const QModelIndex & i, const QVariant & data, int role ) {
	if( i.column() == 0 && role == Qt::CheckStateRole ) {
		checked = Qt::CheckState(data.toInt());
		return true;
	}
	return false;
}

int ServiceCheckItem::compare( const QModelIndex & a, const QModelIndex & b, int column, bool asc) const {
	ServiceCheckItem & other = ServiceCheckModel::data(b);
	return compareRetI(e.service(), other.e.service());
}

ServiceCheckTree::ServiceCheckTree( QWidget * parent )
: RecordTreeView( parent )
, mReadOnly( false )
{
	QStringList headerLabels;
	headerLabels << "Name";

        RecordSuperModel * scm = new RecordSuperModel(this);
        new ServiceCheckModel( scm->treeBuilder() );
        scm->setAutoSort( true );
	scm->setHeaderLabels( headerLabels );
	scm->setRootList( Service::select() );
        setModel( scm );

	setSelectionMode( QAbstractItemView::ExtendedSelection );
	Table * et = Service::table();
	connect( et, SIGNAL( added( RecordList ) ), SLOT( slotElementsAdded( RecordList ) ) );
	connect( et, SIGNAL( removed( RecordList ) ), SLOT( slotElementsRemoved( RecordList ) ) );
	connect( et, SIGNAL( updated( Record, Record ) ), SLOT( slotElementUpdated( Record, Record ) ) );
	setColumnAutoResize( 0, true );
	mService = Service::select();
}

void ServiceCheckTree::setRootElement()
{
	model()->setRootList( mService );
	expandRecursive();
}

ServiceList ServiceCheckTree::elementsByState( Qt::CheckState state )
{
	ServiceList ret;
	RecordSuperModel * m = model();
	for( ModelIter it(m,ModelIter::Recursive); it.isValid(); ++it )
		if( m->data(*it, Qt::CheckStateRole) == state )
			ret += m->getRecord(*it);
	return ret;
}

ServiceList ServiceCheckTree::checkedElements()
{
	return elementsByState( Qt::Checked );
}

ServiceList ServiceCheckTree::noChangeElements()
{
	return elementsByState( Qt::PartiallyChecked );
}

void ServiceCheckTree::setChecked( ServiceList els )
{
	RecordSuperModel * m = model();
	for( ModelIter it(m,ModelIter::Recursive); it.isValid(); ++it )
		if( els.contains( m->getRecord(*it) ) )
			m->setData(*it,Qt::Checked,Qt::CheckStateRole);
}

void ServiceCheckTree::setNoChange( ServiceList els )
{
	RecordSuperModel * m = model();
	for( ModelIter it(m,ModelIter::Recursive); it.isValid(); ++it )
		if( els.contains( m->getRecord(*it) ) )
			m->setData(*it,Qt::PartiallyChecked,Qt::CheckStateRole);
}

void ServiceCheckTree::slotElementsAdded( RecordList  )
{
}

void ServiceCheckTree::slotElementUpdated( Record , Record )
{
}

void ServiceCheckTree::slotElementsRemoved( RecordList  )
{
}

