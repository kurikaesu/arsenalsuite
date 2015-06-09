
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
 * $Id: recordlistmodel.cpp 6490 2008-05-02 02:09:58Z newellm $
 */

#include "recordlistmodel.h"
#include "qvariantcmp.h"

// Forwards hasChildren calls back to RecordListModel's children function
class RecordListModelTreeBuilder : public ModelTreeBuilder
{
public:
	RecordListModelTreeBuilder( SuperModel * model ) : ModelTreeBuilder(model) {}
	virtual bool hasChildren( const QModelIndex & parentIndex, SuperModel * model );
};

bool RecordListModelTreeBuilder::hasChildren( const QModelIndex & parentIndex, SuperModel * model )
{
	RecordListModel * rlm = (RecordListModel*)model;
	RecordList children = rlm->children( rlm->getRecord(parentIndex) );
	if( !children.isEmpty() ) {
		rlm->append(children,parentIndex);
		return true;
	}
	return false;
}

struct RecordListModelItem : public RecordItem
{
	QVariant modelData( const QModelIndex & idx, int role );
	bool setModelData( const QModelIndex & idx, const QVariant & value, int role );
	Qt::ItemFlags modelFlags( const QModelIndex & idx );
	int compare( const QModelIndex & idx, const QModelIndex & idx2, int column, bool asc );

	const RecordListModel * _model(const QModelIndex & idx) { return qobject_cast<const RecordListModel*>(idx.model()); }
};

typedef TemplateRecordDataTranslator<RecordListModelItem,RecordDataTranslatorBase> RecordListModelDataTranslator;

QVariant RecordListModelItem::modelData( const QModelIndex & idx, int role )
{
	const RecordListModel * model = _model(idx);
	return model ? model->recordData(record,role,model->columnName(idx.column())) : QVariant();
}

bool RecordListModelItem::setModelData( const QModelIndex & idx, const QVariant & value, int role )
{
	const RecordListModel * model = _model(idx);
	return model ? const_cast<RecordListModel*>(model)->setRecordData(record,model->columnName(idx.column()),value,role) : false;
}

Qt::ItemFlags RecordListModelItem::modelFlags( const QModelIndex & idx )
{
	const RecordListModel * model = _model(idx);
	return model ? model->recordFlags(record,model->columnName(idx.column())) : Qt::ItemFlags(0);
}

int RecordListModelItem::compare( const QModelIndex & idx, const QModelIndex & idx2, int column, bool asc )
{
	const RecordListModel * model = _model(idx);
	if( model )
		return model->compare(record,model->columnName(column),RecordListModelDataTranslator::data(idx2).record,model->columnName(column));
	return RecordItem::compare( idx, idx2, column, asc );
}

RecordListModel::RecordListModel( QObject * parent )
: RecordSuperModel( parent )
{
	new RecordListModelDataTranslator(treeBuilder());
}

QString RecordListModel::columnName( int col ) const
{
	if( col < mColumns.size() ) return mColumns[col];
	return QString();
}

QVariant RecordListModel::recordData( const Record & record, int role, const QString & column ) const
{
	if( role == Qt::DisplayRole || role == Qt::EditRole )
		return record.getValue( column );
	return QVariant();
}

void RecordListModel::setRecordList( RecordList rl )
{
	setRootList( rl );
}

RecordList RecordListModel::recordList()
{
	return rootList();
}

void RecordListModel::setColumn( const QString & column )
{
	setColumns( QStringList(column) );
}

QString RecordListModel::column()
{
	return mColumns.size() ? mColumns[0] : QString();
}

void RecordListModel::setColumns( QStringList columns )
{
	mColumns = columns;
	setHeaderLabels( columns );
}

QStringList RecordListModel::columns()
{
	return mColumns;
}

Qt::ItemFlags RecordListModel::recordFlags( const Record & /*record*/, const QString & /*column*/ ) const
{
	return Qt::ItemFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
}

bool RecordListModel::setRecordData( const Record & /*record*/, const QString & /*col*/, const QVariant &, int )
{
	return false;
}

int RecordListModel::compare( const Record & r1, const QString & column1, const Record & r2, const QString & column2 ) const
{
	return qVariantCmp( recordData( r1, Qt::DisplayRole, column1 ), recordData( r2, Qt::DisplayRole, column2 ) );
}

