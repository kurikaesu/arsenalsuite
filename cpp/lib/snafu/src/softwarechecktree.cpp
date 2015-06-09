
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
 * $Id: softwarechecktree.cpp 9407 2010-03-03 22:17:59Z brobison $
 */

#include "qvariantcmp.h"

#include "software.h"
#include "softwarechecktree.h"
#include "host.h"
#include "hostsoftware.h"
#include "modeliter.h"

void SoftwareCheckItem::setup( const Record & r, const QModelIndex & ) {
	e = r;
	checked = Qt::Unchecked;
}
QVariant SoftwareCheckItem::modelData( const QModelIndex & i, int role ) {
	if( i.column() == 0 ) {
		if( role == Qt::CheckStateRole )
			return checked;
		else if( role == Qt::DisplayRole )
			return e.name();
	}
	if( i.column() == 1 ) {
		if( role == Qt::DisplayRole )
			return e.version();
	}
	return QVariant();
}
Qt::ItemFlags SoftwareCheckItem::modelFlags( const QModelIndex & ) {
	return Qt::ItemFlags( Qt::ItemIsUserCheckable | Qt::ItemIsEnabled );
}
bool SoftwareCheckItem::setModelData( const QModelIndex & i, const QVariant & data, int role ) {
	if( i.column() == 0 && role == Qt::CheckStateRole ) {
		checked = Qt::CheckState(data.toInt());
		return true;
	}
	return false;
}
int SoftwareCheckItem::compare( const QModelIndex & a, const QModelIndex & b, int column, bool asc) const {
	SoftwareCheckItem & other = SoftwareCheckModel::data(b);
	return compareRetI(e.name(), other.e.name());
}

SoftwareCheckTree::SoftwareCheckTree( QWidget * parent )
: RecordTreeView( parent )
, mReadOnly( false )
{
	QStringList headerLabels;
	headerLabels << "Name" << "Version";
	RecordSuperModel * scm = new RecordSuperModel( this );
	mSoftware = Software::select();
	new SoftwareCheckModel( scm->treeBuilder() );
	scm->setHeaderLabels( headerLabels );
	scm->setRootList( mSoftware );
	setModel( scm );
	setSelectionMode( QAbstractItemView::ExtendedSelection );
	Table * et = Software::table();
	connect( et, SIGNAL( added( RecordList ) ), SLOT( slotElementsAdded( RecordList ) ) );
	connect( et, SIGNAL( removed( RecordList ) ), SLOT( slotElementsRemoved( RecordList ) ) );
	connect( et, SIGNAL( updated( Record, Record ) ), SLOT( slotElementUpdated( Record, Record ) ) );
	setColumnAutoResize( 0, true );
}

void SoftwareCheckTree::setRootElement()
{
	model()->setRootList( mSoftware );
	expandRecursive();
}

SoftwareList SoftwareCheckTree::elementsByState( Qt::CheckState state )
{
	SoftwareList ret;
	RecordSuperModel * m = model();
	for( ModelIter it(m,ModelIter::Recursive); it.isValid(); ++it )
		if( m->data(*it, Qt::CheckStateRole) == state )
			ret += m->getRecord(*it);
	return ret;
}

SoftwareList SoftwareCheckTree::checkedElements()
{
	return elementsByState( Qt::Checked );
}

SoftwareList SoftwareCheckTree::noChangeElements()
{
	return elementsByState( Qt::PartiallyChecked );
}

void SoftwareCheckTree::setChecked( SoftwareList els )
{
	RecordSuperModel * m = model();
	for( ModelIter it(m,ModelIter::Recursive); it.isValid(); ++it )
		if( els.contains( m->getRecord(*it) ) )
			m->setData(*it,Qt::Checked,Qt::CheckStateRole);
}

void SoftwareCheckTree::setNoChange( SoftwareList els )
{
	RecordSuperModel * m = model();
	for( ModelIter it(m,ModelIter::Recursive); it.isValid(); ++it )
		if( els.contains( m->getRecord(*it) ) )
			m->setData(*it,Qt::PartiallyChecked,Qt::CheckStateRole);
}

void SoftwareCheckTree::slotElementsAdded( RecordList  )
{
}

void SoftwareCheckTree::slotElementUpdated( Record , Record )
{
}

void SoftwareCheckTree::slotElementsRemoved( RecordList  )
{
}

