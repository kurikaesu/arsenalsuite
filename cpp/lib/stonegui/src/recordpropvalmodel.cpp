
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
 * $Id: recordpropvalmodel.cpp 11739 2011-06-24 00:05:10Z newellm $
 */

#include "recordpropvalmodel.h"
#include "table.h"

class PropValItem : public ItemBase
{
public:
	int column;
	int row;
	RecordList records;
	bool isfkey;
	mutable Record fkey;
	QString prop;

	PropValItem();

	void setup( int c, const RecordList & recs, int r=-1 );

	QVariant modelData( const QModelIndex & i, int role ) const;

	bool setModelData( const QModelIndex & i, const QVariant & v, int role );

	Qt::ItemFlags modelFlags( const QModelIndex & i );

	Record getRecord() { return Record(); }
};

typedef TemplateDataTranslator<PropValItem> RecordPropValTranslator;

PropValItem::PropValItem()
: column( 0 )
, row( -1 )
, isfkey( false )
{}

void PropValItem::setup( int c, const RecordList & recs, int r )
{
	column = c;
	row = r;
	records = recs;
	if( recs.size() ) {
		TableSchema * s = records[0].table()->schema();
		if( s ) {
			Field * f = s->field(column);
			if( f ) {
				prop = f->displayName();
				TableSchema * fkt = f->foreignKeyTable();
				isfkey = fkt && fkt->field( "name" );
			}
		}
	}
}

QVariant PropValItem::modelData( const QModelIndex & i, int role ) const
{
	if( records.isEmpty() ) return QVariant();
	
	if( i.column() == 0 && role == Qt::DisplayRole ) {
		return prop;
	} else if( i.column() == 1 ) {
		if( role == Qt::DisplayRole || role == Qt::EditRole ) {
			if( (row >= 0 && row < (int)records.size()) || records.size() == 1 ) {
				Record r = records[row >= 0 ? row : 0];
				if( isfkey && role == Qt::DisplayRole ) {
					fkey = r.foreignKey(column);
					return fkey.getValue( "name" );
				}
				return r.getValue( column );
			} else if( row == -1 ) {
				QVariantList vals;
				if( isfkey )
					vals = records.foreignKey(column).getValue("name");
				else
					vals = records.getValue(column);
				QStringList strings;
				foreach( QVariant v, vals ) {
					QString s = v.toString();
					if( !s.isEmpty() && !strings.contains(s) )
						strings += s;
				}
				return strings.join(",");
			}
		}
	}
	return QVariant();
}

bool PropValItem::setModelData( const QModelIndex & i, const QVariant & v, int role )
{
	int col = i.column();
	if( col == 0 ) return false;
	if( col == 1 ) {
		if( role == Qt::EditRole ) {
			if( row == -1 ) {
				records.setValue( column, v );
				records.commit();
				return true;
			} else if( row >= 0 && row < (int)records.size() ) {
				Record r = records[row];
				r.setValue( column, v );
				r.commit();
				return true;
			}
		}
	}
	return false;
}

Qt::ItemFlags PropValItem::modelFlags( const QModelIndex & i )
{
	switch( i.column() ) {
		case 0:
			return Qt::ItemFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
		case 1:
		{
			const RecordPropValModel * rpvm = qobject_cast<const RecordPropValModel*>(i.model());
			// Only allow editting if the model is allowed to edit, and it is a single value
			Qt::ItemFlag editable = (rpvm->mMultiEdit || row < 0) && rpvm && rpvm->mEditable ? Qt::ItemIsEditable : Qt::ItemFlag(0);
			return Qt::ItemFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled | editable );
		}
	}
	return Qt::ItemFlags();
}

class RecordPropValTreeBuilder : public ModelTreeBuilder
{
public:
	RecordPropValTreeBuilder( SuperModel * model );

	bool hasChildren( const QModelIndex & parentIndex, SuperModel * model );
	void loadChildren( const QModelIndex & parentIndex, SuperModel * model );

	RecordList records;
};

RecordPropValTreeBuilder::RecordPropValTreeBuilder( SuperModel * model )
: ModelTreeBuilder( model )
{
	new RecordPropValTranslator(this);
}

bool RecordPropValTreeBuilder::hasChildren( const QModelIndex & parentIndex, SuperModel * model )
{
	Q_UNUSED(model);

	if( RecordPropValTranslator::isType(parentIndex) ) {
		PropValItem & pvi = RecordPropValTranslator::data(parentIndex);
		return pvi.records.size() > 1 && pvi.row == -1;
	}
	return false;
}

void RecordPropValTreeBuilder::loadChildren( const QModelIndex & parentIndex, SuperModel * model )
{
	if( records.isEmpty() ) return;
	
	Record r = records[0];
	TableSchema * ts = r.table()->schema();

	if( !ts ) return;

	int cnt = ts->fieldCount();
	model->clearChildren(parentIndex);
	model->append(parentIndex,cnt);
	for( int i=0; i<cnt; i++ ) {
		QModelIndex idx = model->index(i,0,parentIndex);
		RecordPropValTranslator::data(idx).setup(i,records);
		if( records.size() > 1 ) {
			model->append(idx,records.size());
			for( int r=0; r < (int)records.size(); r++ )
				RecordPropValTranslator::data(idx.child(r,0)).setup(i,records,r);
		}
	}
	return;
}

RecordPropValModel::RecordPropValModel( QObject * parent )
: SuperModel( parent )
, mEditable( false )
, mMultiEdit( false )
{
	setTreeBuilder( new RecordPropValTreeBuilder(this) );
	setHeaderLabels( QStringList() << "Property" << "Value" );
}

void RecordPropValModel::setRecords( const RecordList & rl )
{
	mRecords = rl;
	mRecords.selectFields();
	((RecordPropValTreeBuilder*)treeBuilder())->records = rl;
	treeBuilder()->loadChildren( QModelIndex(), this );
}

RecordList RecordPropValModel::records() const
{
	return mRecords;
}

void RecordPropValModel::setEditable( bool editable )
{
	mEditable = editable;
}

bool RecordPropValModel::editable() const
{
	return mEditable;
}

void RecordPropValModel::setMultiEdit( bool m )
{
	mMultiEdit = m;
}

bool RecordPropValModel::multiEdit() const
{
	return mMultiEdit;
}

RecordList RecordPropValModel::foreignKeyRecords( const QModelIndex & index )
{
	if( RecordPropValTranslator::isType(index) ) {
		PropValItem * item = &RecordPropValTranslator::data(index);
		RecordList records = item->records;
		if( item->row >= 0 && item->row < (int)records.size() )
			return records[item->row].foreignKey(item->column);
		return records.foreignKey(item->column);
	}
	return RecordList();
}

