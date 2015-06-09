
#include <qtimer.h>
#include <qregexp.h>

#include "blurqt.h"
#include "modelgrouper.h"
#include "supermodel.h"

void GroupItem::recalculateGroupValues( const QModelIndex & self )
{
	QAbstractItemModel * model = const_cast<QAbstractItemModel*>(self.model());
	for( int i = model->columnCount(self) - 1; i >= 0; --i ) {
		QModelIndex ci = self.sibling(self.row(),i);
		QString val = calculateGroupValue( self, i );
		if( !val.isEmpty() )
			setModelData( ci, val, Qt::DisplayRole );
	}
}

QString GroupItem::calculateGroupValue( const QModelIndex & self, int column)
{
	if( column == groupColumn )
		return QString( "%1 (%2 rows)" ).arg(groupValue).arg(self.model()->rowCount(self));
	/*QAbstractItemModel * model = self.model();
	for( int i = model->rowCount(self) - 1; i >= 0; --i ) {
		QModelIndex ci = self.child(i,column);
	}*/
	return QString();
}

QVariant GroupItem::modelData( const QModelIndex & i, int role ) const
{
	if( role == ModelGrouper::GroupingColumn )
		return groupColumn;
	if( role == ModelGrouper::GroupingValue )
		return groupValue;
	return StandardItem::modelData(i,role);
}

bool GroupItem::setModelData( const QModelIndex & self, const QVariant & value, int role )
{
	if( role == ModelGrouper::GroupingUpdate ) {
		recalculateGroupValues(self);
		return true;
	}	
	if( role == ModelGrouper::GroupingColumn ) {
		groupColumn = value.toInt();
		return true;
	}
	if( role == ModelGrouper::GroupingValue ) {
		groupValue = value.toString();
		return true;
	}
	return StandardItem::setModelData(self,value,role);
}

ModelGrouper::ModelGrouper( SuperModel * model )
: QObject( model )
, mModel( model )
, mTranslator( 0 )
, mGroupColumn( 0 )
, mIsGrouped( false )
, mInsertingGroupItems( false )
, mUpdateScheduled( false )
, mEmptyGroupPolicy( HideEmptyGroups )
, mExpandNewGroups( true )
{
	model->setGrouper(this);
	connect( model, SIGNAL( rowsInserted(const QModelIndex &, int, int) ), SLOT( slotRowsInserted( const QModelIndex &, int, int) ) );
	connect( model, SIGNAL( rowsRemoved(const QModelIndex &, int, int) ), SLOT( slotRowsRemoved( const QModelIndex &, int, int) ) );
	connect( model, SIGNAL( dataChanged(const QModelIndex &, const QModelIndex &) ), SLOT( slotDataChanged(const QModelIndex &, const QModelIndex &) ) );
}

void ModelGrouper::setColumnGroupRegex( int column, const QRegExp & regEx )
{
	if( regEx.isValid() )
		mColumnRegexMap[column] = regEx;
	else
		mColumnRegexMap.remove(column);
}

QRegExp ModelGrouper::columnGroupRegex( int column ) const
{
	ColumnRegexMap::const_iterator it = mColumnRegexMap.find( column );
	if( it == mColumnRegexMap.end() ) return QRegExp();
	return it.value();
}

void ModelGrouper::setColumnGroupRole( int column, int role )
{
	if( role != Qt::DisplayRole )
		mColumnRoleMap[column] = role;
	else
		mColumnRoleMap.remove(column);
}

int ModelGrouper::columnGroupRole(int column) const
{
	ColumnRoleMap::const_iterator it = mColumnRoleMap.find(column);
	if( it == mColumnRoleMap.end() ) return Qt::DisplayRole;
	return it.value();
}

ModelDataTranslator * ModelGrouper::groupedItemTranslator() const
{
	if( !mTranslator ) {
		// I could make it mutable, but what fun would that be?
		ModelGrouper * me = const_cast<ModelGrouper*>(this);
		me->mTranslator = new GroupTranslator(model()->treeBuilder());
	}
	return mTranslator;
}

void ModelGrouper::setGroupedItemTranslator( ModelDataTranslator * trans )
{
	if( trans == mTranslator ) return;
	bool grouped = mIsGrouped;
	if( grouped )
		ungroup();
	// Because all ModelDataTranslators are owned by the treeBuilder(mTranslators list),
	// we don't have to do any memory management here
	mTranslator = trans;
	if( grouped )
		groupByColumn(mGroupColumn);
}


ModelGrouper::EmptyGroupPolicy ModelGrouper::emptyGroupPolicy() const
{
	return mEmptyGroupPolicy;
}

void ModelGrouper::setEmptyGroupPolicy( ModelGrouper::EmptyGroupPolicy policy )
{
	if( mEmptyGroupPolicy != policy ) {
		mEmptyGroupPolicy = policy;
		// TODO: Remove empty groups if needed
	}
}

void ModelGrouper::setExpandNewGroups(bool expandNewGroups)
{
	mExpandNewGroups = expandNewGroups;
}

bool ModelGrouper::expandNewGroups() const
{
	return mExpandNewGroups;
}

void ModelGrouper::groupByColumn( int column ) {
	// Single level grouping for now
	if( mIsGrouped ) ungroup();
	
	mGroupColumn = column;
	mGroupRole = columnGroupRole(column);
	mGroupRegEx = columnGroupRegex(column);
	int count = model()->rowCount();
	if( count > 0 )
		groupRows( 0, count - 1 );
	
	mIsGrouped = true;
	emit grouped();
	emit groupingChanged( true );
}

QModelIndexList fromPersist( QList<QPersistentModelIndex> persist)
{
	QModelIndexList ret;
	foreach( QModelIndex idx, persist ) ret.append(idx);
	return ret;
}

QString indexToStr( const QModelIndex & idx )
{ return QString("(%1,%2,%3)").arg(idx.row()).arg(idx.column()).arg((qulonglong)idx.internalPointer(),0,16); }

QString indexListToStr( QModelIndexList list )
{
	QStringList rows;
	foreach( QModelIndex idx, list ) rows += indexToStr(idx);
	return rows.join(",");
}

QString ModelGrouper::groupValue( const QModelIndex & idx )
{
	QString strValue = model()->data( idx.column() == mGroupColumn ? idx : idx.sibling(idx.row(),mGroupColumn), mGroupRole ).toString();
	if( !mGroupRegEx.isEmpty() && mGroupRegEx.isValid() && strValue.contains(mGroupRegEx) )
		strValue = mGroupRegEx.cap(mGroupRegEx.captureCount() > 1 ? 1 : 0);
	//LOG_5( QString("Index %1 grouped with value %2").arg(indexToStr(idx)).arg(strValue) );
	return strValue;
}

void ModelGrouper::groupRows( int start, int end )
{
	// First group the top level indexes by the desired column
	GroupMap grouped;
	for( int i = start; i <= end; ++i ) {
		QModelIndex idx = model()->index( i, mGroupColumn );
		// Double check that this isn't a group item, even though it shouldn't be
		if( model()->translator(idx) != groupedItemTranslator() ) {
			grouped[groupValue(idx)] += idx;
		}
	}
	group( grouped );
}
	
void ModelGrouper::group( GroupMap & grouped )
{
	QList<QPersistentModelIndex> persistentGroupIndexes;
	
	// If we are already grouped, we need to insert items into existing groups before creating new ones
	if( mIsGrouped ) {
		// Get persistent indexes for each group item, because regular ones may be invalidated by 
		// the move call in the loop
		for( ModelIter it(model()); it.isValid(); ++it )
			if( model()->translator(*it) == groupedItemTranslator() )
				persistentGroupIndexes.append( *it );
		foreach( QPersistentModelIndex idx, persistentGroupIndexes ) {
			bool isEmptyGroup = model()->rowCount(idx) == 0;
			QString groupVal = idx.sibling( idx.row(), mGroupColumn ).data( ModelGrouper::GroupingValue ).toString();
			GroupMap::Iterator mapIt = grouped.find( groupVal );
			if( mapIt != grouped.end() ) {
				QModelIndexList toMove(fromPersist(mapIt.value()));
				//LOG_5( QString("Moving indexes %1 to existing group item at index %2").arg(indexListToStr(toMove)).arg(indexToStr(idx)) );
				model()->move( toMove, idx );
				if( isEmptyGroup )
					emit groupPopulated( idx );
				if( mUpdateScheduled ) {
					if( !mGroupItemsToUpdate.contains( idx ) )
						mGroupItemsToUpdate.append(idx);
				} else
					// Tell the group item to update itself based on the added children
					model()->setData( idx, QVariant(), GroupingUpdate );
				grouped.erase( mapIt );
			}
		}
		// Deal with any now-empty groups
		for( QList<QPersistentModelIndex>::Iterator it = persistentGroupIndexes.begin(); it != persistentGroupIndexes.end(); )
			if( model()->translator(*it) == groupedItemTranslator() && model()->rowCount(*it) == 0 ) {
				emit groupEmptied(*it);
				++it;
			} else
				it = persistentGroupIndexes.erase( it );
		
		if( emptyGroupPolicy() == RemoveEmptyGroups )
			model()->remove( fromPersist( persistentGroupIndexes ) );
	}
	
	if( grouped.size() ) {
		// Append the group items, yet to be filled with data and have the children copied
		{
			mInsertingGroupItems = true;
			QModelIndexList groupIndexes = model()->insert( QModelIndex(), model()->rowCount(), grouped.size(), groupedItemTranslator() );
			//LOG_5( QString("Created %1 new group items at indexes %2").arg(grouped.size()).arg(indexListToStr(groupIndexes)) );
			mInsertingGroupItems = false;
			foreach( QModelIndex idx, groupIndexes ) persistentGroupIndexes.append(idx);
		}
		
		// Set the group column data, and copy the children
		int i = 0;
		for( QMap<QString, QList<QPersistentModelIndex> >::Iterator it = grouped.begin(); it != grouped.end(); ++it, ++i ) {
			QModelIndex groupIndex = persistentGroupIndexes[i];
			QModelIndexList toMove(fromPersist(it.value()));
			//LOG_5( QString("Moving indexes %1 to existing group item at index %2").arg(indexListToStr(toMove)).arg(indexToStr(groupIndex)) );
			model()->move( toMove, groupIndex );
		}
		
		i = 0;
		for( QMap<QString, QList<QPersistentModelIndex> >::Iterator it = grouped.begin(); it != grouped.end(); ++it, ++i ) {
			QModelIndex groupIndex = persistentGroupIndexes[i];
			model()->setData( groupIndex, QVariant(mGroupColumn), GroupingColumn );
			model()->setData( groupIndex, QVariant(it.key()), GroupingValue );
			model()->setData( groupIndex, QVariant(), GroupingUpdate );
			// Emit this here(as opposed to above) so that any slots can get the group value by calling groupValue(idx)
			emit groupCreated( persistentGroupIndexes[i] );
		}
	}
}

void ModelGrouper::ungroup()
{
	// If we disable auto sort, we can ensure that the group indexes wont change
	// because the new indexes will be appended
	bool autoSort = model()->autoSort();
	model()->setAutoSort( false );

	// Clear this here so that when we start moving the items back to root level
	// the slotRowsInserted doesn't try to regroup them
	mIsGrouped = false;
	
	// Collect a list the top level(group) items, and the second level items
	QModelIndexList topLevel;
	QModelIndexList children;
	for( ModelIter it(model()); it.isValid(); ++it ) {
		if( model()->translator(*it) == groupedItemTranslator() ) {
			topLevel += *it;
			children += ModelIter::collect( (*it).child(0,0) );
		}
	}
	// Copy the second level items to root level
	model()->move( children, QModelIndex() );
	
	// Remove the group items
	model()->remove( topLevel );

	if( autoSort ) {
		model()->setAutoSort( true );
		model()->resort();
	}

	emit ungrouped();
	emit groupingChanged( false );
}


void ModelGrouper::slotRowsInserted( const QModelIndex & parent, int start, int end )
{
	// Move any new rows into their proper groups
	if( mIsGrouped && !parent.isValid() && !mInsertingGroupItems )
		groupRows( start, end );
}

void ModelGrouper::slotRowsRemoved( const QModelIndex & parent, int, int )
{
	if( mIsGrouped && parent.isValid() && model()->translator(parent) == groupedItemTranslator() ) {
		if( model()->rowCount(parent) == 0 ) {
			emit groupEmptied( parent );
			if( emptyGroupPolicy() == RemoveEmptyGroups )
				model()->remove( parent );
		} else {
			if( !mGroupItemsToUpdate.contains( parent ) )
				mGroupItemsToUpdate += parent;
			scheduleUpdate();
		}
	}
}

void ModelGrouper::slotDataChanged( const QModelIndex & topLeft, const QModelIndex & bottomRight )
{
	if( !mIsGrouped ) return;
	QModelIndex parent = topLeft.parent();
	if( parent.isValid() && model()->translator(parent) == groupedItemTranslator() ) {
		if( !mGroupItemsToUpdate.contains( parent ) )
			mGroupItemsToUpdate += parent;
		scheduleUpdate();
		if( topLeft.column() <= mGroupColumn && bottomRight.column() >= mGroupColumn ) {
			QString parGroupValue = parent.sibling( parent.row(), mGroupColumn ).data( ModelGrouper::GroupingValue ).toString();
			QModelIndex it = topLeft, end = bottomRight;
			QList<QPersistentModelIndex> changed;
			while( it.isValid() && it.row() <= end.row() ) {
				if( groupValue(it) != parGroupValue ) {
					QPersistentModelIndex pit(it);
					if( !mItemsToRegroup.contains(pit) )
						mItemsToRegroup += pit;
				}
				it = it.sibling( it.row() + 1, it.column() );
			}
			// Somewhat inefficient way to regroup, put them toplevel then the slotRowsInserted trigger will
			// move them. Need to rearrange groupRowsByColumn to avoid this.
			if( mItemsToRegroup.size() )
				scheduleUpdate();
		}
	}
}

void ModelGrouper::slotUpdate()
{
	if( mItemsToRegroup.size() ) {
		// First group the top level indexes by the desired column
		GroupMap grouped;
		foreach( QModelIndex idx, mItemsToRegroup ) {
			// Double check that this isn't a group item, even though it shouldn't be
			if( model()->translator(idx) != groupedItemTranslator() )
				grouped[groupValue(idx)] += idx;
		}
		group( grouped );
		mItemsToRegroup.clear();
	}

	if( mGroupItemsToUpdate.size() ) {
		foreach( QModelIndex i, mGroupItemsToUpdate )
			if( i.isValid() ) {
				model()->setData( i, QVariant(), GroupingUpdate );
			}
		mGroupItemsToUpdate.clear();
	}
	
	mUpdateScheduled = false;
}

void ModelGrouper::scheduleUpdate()
{
	if( !mUpdateScheduled ) {
		mUpdateScheduled = true;
		QTimer::singleShot( 0, this, SLOT( slotUpdate() ) );
	}
}
