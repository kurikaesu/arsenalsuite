
#include <algorithm>

#include <qmap.h>
#include <qpair.h>
#include <qvariant.h>

#include "interval.h"
#include "qvariantcmp.h"

#include "modelgrouper.h"
#include "supermodel.h"

#include <stdio.h>
#include <typeinfo>


ModelTreeBuilder::ModelTreeBuilder( SuperModel * parent )
: QObject( parent )
, mModel( parent )
, mLoadingNode( 0 )
{
}

ModelTreeBuilder::~ModelTreeBuilder()
{
	foreach( ModelDataTranslator * trans, mTranslators )
		delete trans;
}

bool ModelTreeBuilder::hasChildren( const QModelIndex &, SuperModel *  )
{
	return false;
}

void ModelTreeBuilder::loadChildren( const QModelIndex & parentIndex, SuperModel * model )
{
	Q_UNUSED(parentIndex);
	Q_UNUSED(model);
}

int ModelTreeBuilder::compare( const QModelIndex & a, const QModelIndex & b, QList<SortColumnPair> columns )
{
	foreach( SortColumnPair sc, columns ) {
		int result = qVariantCmp( a.sibling(a.row(),sc.first).data(Qt::DisplayRole), b.sibling(b.row(),sc.first).data(Qt::DisplayRole) );
		if( result != 0 ) return result;
	}
	return 0;
}

void ModelTreeBuilder::_loadChildren( const QModelIndex & parentIndex, SuperModel * model, ModelNode * node )
{
	ModelNode * previousLoadingNode = mLoadingNode;
	mLoadingNode = node;
	node->setTranslator(defaultTranslator());
	loadChildren(parentIndex,model);	
	mLoadingNode = previousLoadingNode;
}

void ModelTreeBuilder::setTranslator( ModelDataTranslator * trans )
{
	if( mLoadingNode && !mLoadingNode->translator() ) mLoadingNode->setTranslator(trans);
}

void ModelTreeBuilder::setDefaultTranslator( ModelDataTranslator * trans )
{
	int idx = mTranslators.indexOf(trans);
	if( idx >= 0 ) {
		mTranslators.removeAt(idx);
		mTranslators.push_front(trans);
	}
}
	
ModelDataTranslator * ModelTreeBuilder::defaultTranslator()
{
	if( !mTranslators.isEmpty() ) return mTranslators[0];
	return 0;
}

void ModelTreeBuilder::addTranslator(ModelDataTranslator * trans)
{
	mTranslators.append(trans);
}

// builder takes ownership of this
ModelDataTranslator::ModelDataTranslator( ModelTreeBuilder * builder )
: mBuilder( builder )
{
	builder->addTranslator(this);
}

QModelIndex ModelDataTranslator::insert( int row, const QModelIndex & parent )
{
	return model()->insert(parent,row,this);
}

QModelIndex ModelDataTranslator::append( const QModelIndex & parent )
{
	return model()->append(parent,this);
}

void * ModelDataTranslator::dataPtr( const QModelIndex & idx )
{ return idx.isValid() ? ((SuperModel*)idx.model())->indexToNode(idx)->itemData(idx) : 0; }

ModelDataTranslator * ModelDataTranslator::translator( const QModelIndex & idx )
{ return idx.isValid() ? ((SuperModel*)idx.model())->translator(idx) : 0; }

int ItemBase::compare( const QModelIndex & idx, const QModelIndex & idx2, int column, bool  ) const
{ 
	Q_UNUSED(column);
	return qVariantCmp(idx.data(Qt::DisplayRole),idx2.data(Qt::DisplayRole)); 
}

StandardItem::StandardItem()
: mFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled )
{}

int StandardItem::findItem(int column, int role ) const {
	for( int i=0; i < mData.size(); ++i ) {
		const DataItem & item = mData[i];
		if( item.column == column && item.role == role )
			return i;
	}
	return -1;
}

QVariant StandardItem::modelData( const QModelIndex & idx, int role ) const
{
	int i = findItem(idx.column(),role);
	return i>=0 ? mData[i].data : QVariant();
}

bool StandardItem::setModelData( const QModelIndex & idx, const QVariant & value, int role )
{
	int i = findItem(idx.column(),role);
	if( i < 0 ) {
		i = mData.size();
		DataItem item;
		item.role = role;
		item.column = idx.column();
		mData.append( item );
	}
	mData[i].data = value;
	return true;
}


inline QModelIndex SuperModel::nodeToIndex( int row, int column, ModelNode * node )
{
	return createIndex( row, column, node );
}

ModelNode::ModelNode( ModelNode * parent, int parentRow )
: mRowCount( 0 )
, mTranslator( 0 )
, mParent( parent )
, mParentRow( parentRow )
{
	if( mParent && !mTranslator ) mTranslator = mParent->mTranslator;
}

ModelNode::~ModelNode()
{
	clear();
}

void ModelNode::clear()
{
	for( int i=rowCount()-1; i>=0; i-- ) {
		ItemInfo ii = rowToItemInfo(i);
		_translator(ii.translatorIndex)->deleteData( _itemData(ii.dataOffset) );
		if( ii.index < (uint)mChildren.size() ) {
			ModelNode * child = mChildren[ii.index];
			if( child && child != (ModelNode*)1 )
				delete child;
		}
	}
	mNoChildrenArray.clear();
	mChildren.clear();

	mItemInfoVector.clear();

	mRowCount = 0;
	mItemData.clear();
	mFreeSpaceMap.clear();
	mChildrenFreeIndexList.clear();
}

void ModelNode::setTranslator( ModelDataTranslator * translator )
{
	mTranslator = translator;
}

bool ModelNode::insertChildren( int index, int count, ModelDataTranslator * translator, bool skipConstruction )
{
	if( index > mRowCount || index < 0 ) return false;

	if( translator == 0 )
		translator = mTranslator;

	int translatorIdx = 0;
	if( mTranslator != translator ) {
		if( mTranslator == 0 ) 
			mTranslator = translator;
		else {
			translatorIdx = mTranslatorList.indexOf(translator);
			if( translatorIdx == -1 ) {
				if( mTranslatorList.isEmpty() ) mTranslatorList.append( mTranslator );
				translatorIdx = mTranslatorList.size();
				mTranslatorList.append(translator);
			}
		}
	}
		
	int dataSize = translator->dataSize();

	QList<int> & freeSlots = mFreeSpaceMap[dataSize];
	int reusedSlots = qMin(freeSlots.size(), count);
	int newSlots = count - reusedSlots;
	int reusedIndexes = qMin(mChildrenFreeIndexList.size(), count);
	int newIndexes = count - reusedIndexes;
	
	// Extend item data
	// TODO: We may want to keep things aligned if size >= 4 bytes
	int startDataOffset = mItemData.size(), startIndex = mNoChildrenArray.size();
	mItemData.resize( mItemData.size() + newSlots * dataSize );
	mItemInfoVector.insert( index, count, ItemInfo() );
	mNoChildrenArray.resize( startIndex + newIndexes );

	for( int i = count-1; i >= 0; --i ) {
		ItemInfo & ii = mItemInfoVector[index+i];
		if( i >= newIndexes ) {
			ii.index = mChildrenFreeIndexList.front();
			mChildrenFreeIndexList.pop_front();
		} else
			ii.index = startIndex + i;
		if( i >= newSlots ) {
			ii.dataOffset = freeSlots.front();
			freeSlots.pop_front();
		} else
			ii.dataOffset = startDataOffset + i * dataSize;
		ii.translatorIndex = translatorIdx;
		if( !skipConstruction )
			translator->constructData( _itemData(ii.dataOffset) );
	}

	mRowCount += count;

	fixupChildParentRows( index + count );

	return true;
}

bool ModelNode::removeChildren( int index, int count, bool stealObject )
{
	if( count < 0 || index < 0 || index + count > mRowCount ) return false;

	for( int i = index + count - 1; i >= index; --i ) {
		ItemInfo ii = rowToItemInfo(i);
		void * data = _itemData( ii.dataOffset );
		ModelDataTranslator * trans = _translator(ii.translatorIndex);
		
		// Store the free space entry
		mFreeSpaceMap[trans->dataSize()] += ii.dataOffset;
		mChildrenFreeIndexList.append( ii.index );
		mNoChildrenArray[ii.index] = false;
		
		// Destruct the data
		if( !stealObject )
			trans->deleteData( data );
		
		if( ii.index < (uint)mChildren.size() ) {
			ModelNode * children = mChildren[ii.index];
			if( children && children != (ModelNode*)1 )
				delete children;
			mChildren[ii.index] = 0;
		}
	}
	mItemInfoVector.remove(index,count);
	mRowCount -= count;
	fixupChildParentRows( index );
	return true;
}

void ModelNode::fixupChildParentRows( int startIndex )
{
	if( startIndex < 0 || startIndex >= mRowCount ) return;
	
	for( int i = startIndex; i < mRowCount; ++i ) {
		int index = rowToItemInfo(i).index;
		if( mChildren.size() <= index ) continue;
		ModelNode * mn = mChildren[index];
		if( mn && mn != (ModelNode*)1 )
			mn->mParentRow = i;
	}
}

bool ModelNode::hasChildren( const QModelIndex & idx, bool insert, bool skipTreeBuilderCheck ) {
	int row = idx.row();
	if( row < 0 || row >= mRowCount ) return false;
	int index = rowToItemInfo(idx.row()).index;
	if( mNoChildrenArray.at(index) ) {
		if( insert ) mNoChildrenArray.clearBit(index);
		else return false;
	}
	if( insert || mChildren.size() <= index || mChildren.at(index) == 0 ) {
		if( skipTreeBuilderCheck )
			return false;
		if( !insert ) {
			bool hasChildren = model(idx)->treeBuilder()->hasChildren(idx,model(idx));
			if( !hasChildren ) {
				mNoChildrenArray.setBit( index, true );
				return false;
			}
		}
		
		if( mChildren.size() <= index )
			mChildren.resize(index + 1);
		if( !mChildren[index] )
			mChildren[index] = (ModelNode*)1;
		return true;
	}
	if( mChildren.size() > index ) {
		ModelNode * node = mChildren.at(index);
		if( node && node != (ModelNode*)1 ) return node->rowCount();
	}
	return true;
}

void ModelNode::setChild( const QModelIndex & idx, ModelNode * childNode )
{
	if( idx.row() < 0 || idx.row() >= mRowCount ) return;
	if( !childNode ) return;
	// Ensure a spot in the array
	ModelNode * current = child(idx,true);
	if( current && current != (ModelNode*)1 )
		delete current;
	mChildren[rowToItemInfo(idx.row()).index] = childNode;
	childNode->mParent = this;
	childNode->mParentRow = idx.row();
}

ModelNode * ModelNode::child( const QModelIndex & idx, bool insert, bool steal )
{
	if( !hasChildren(idx,insert,steal) ) return 0;
	// If hasChildren returns true, then we at least have a spot in mChildren
	int index = rowToItemInfo(idx.row()).index;
	ModelNode * node = mChildren[index];
	if( node == (ModelNode*)1 ) {
		node = new ModelNode(this,idx.row());
		mChildren[index] = node;
		// Should we give the treebuilder a chance to load children
		// if insert has been called but these children were never
		// asked for, therefore never given a chance to be loaded
		// by treeBuilder?  That at least means the caller of insert
		// has not even called rowCount, so they are inserting at row 0
		if( !insert && !steal )
			model(idx)->treeBuilder()->_loadChildren(idx,model(idx),node);
		return node;
	}
	if( steal )
		mChildren[index] = 0;
	return node;
}

bool ModelNode::childrenLoaded( const QModelIndex & idx ) {
	int row = idx.row();
	if( row < 0 || row >= mRowCount ) return false;
	int index = rowToItemInfo(row).index;
	if( mNoChildrenArray.at(index) )
		return true;
	if( index < mChildren.size() ) {
		ModelNode * node = mChildren.at(index);
		if( node == 0 || node == (ModelNode*)1 ) return false;
		return true;
	}
	return false;
}

void ModelNode::clearChildren( const QModelIndex & idx ) {
	int row = idx.row();
	if( row < 0 || row >= mRowCount ) return;
	int index = rowToItemInfo(row).index;
	if( mNoChildrenArray.at(index) ) {
		return;
	}
	if( index < mChildren.size() ) {
		ModelNode * child = mChildren.at(index);
		delete child;
		mChildren[index] = 0;
	}
}

void ModelNode::swap( const QModelIndex & a, const QModelIndex & b )
{
	ItemInfo & info_a = rowToItemInfo(a.row()), & info_b = rowToItemInfo(b.row());
	ModelNode * child_a = mChildren.size() > info_a.index ? mChildren[info_a.index] : 0;
	ModelNode * child_b = mChildren.size() > info_b.index ? mChildren[info_b.index] : 0;
	if( child_a && child_a != (ModelNode*)1 ) child_a->mParentRow = b.row();
	if( child_b && child_b != (ModelNode*)1 ) child_b->mParentRow = a.row();
	ItemInfo copy = info_a;
	info_a = info_b;
	info_b = copy;
	SuperModel * sm = model(a);
	QModelIndexList from, to;
	for( int i=sm->columnCount(a)-1; i>=0; i--) {
		from << a.sibling(a.row(), i);
		to << a.sibling(b.row(), i);

		from << a.sibling(b.row(), i);
		to << a.sibling(a.row(), i);
	}
	sm->changePersistentIndexList(from,to);
}

inline int ModelNode::rowCount()
{
	return mRowCount;
}

inline ModelNode::ItemInfo & ModelNode::rowToItemInfo(int row)
{
	return mItemInfoVector[row];
}

inline ModelDataTranslator * ModelNode::_translator(int translatorIndex) const
{
	if( translatorIndex == 0 ) return mTranslator;
	return mTranslatorList[translatorIndex];
}

ModelDataTranslator * ModelNode::translator(const QModelIndex & idx)
{
	int row = idx.row();
	if( idx.internalPointer() == this && row >= 0 && row < mRowCount )
		return _translator(rowToItemInfo(row).translatorIndex);
	return 0;
}

inline void * ModelNode::_itemData( int dataOffset )
{
	return &mItemData.data()[dataOffset];
}

inline QVariant ModelNode::data( const QModelIndex & idx, int role ) {
	int row = idx.row();
	if( row < 0 || row >= mRowCount ) return QVariant();
	ItemInfo ii = rowToItemInfo(row);
	return _translator(ii.translatorIndex)->modelData( _itemData(ii.dataOffset), idx, role );
}

bool ModelNode::setData( const QModelIndex & idx, const QVariant & value, int role ) {
	int row = idx.row();
	if( row < 0 || row >= mRowCount ) return false;
	ItemInfo ii = rowToItemInfo(row);
	return _translator(ii.translatorIndex)->setModelData( _itemData(ii.dataOffset), idx, value, role );
}

inline Qt::ItemFlags ModelNode::flags( const QModelIndex & idx ) {
	int row = idx.row();
	if( row < 0 || row >= mRowCount ) return Qt::ItemFlags(0);
	ItemInfo ii = rowToItemInfo(row);
	return _translator(ii.translatorIndex)->modelFlags( _itemData(ii.dataOffset), idx );
}

SuperModel * ModelNode::model(const QModelIndex &idx)
{
	return const_cast<SuperModel*>(reinterpret_cast<const SuperModel*>(idx.model()));
}

inline void * ModelNode::itemData( const QModelIndex & idx )
{
	int row = idx.row();
	if( idx.internalPointer() == this && row >= 0 && row < mRowCount )
		return _itemData(rowToItemInfo(row).dataOffset);
	return 0;
}

int ModelNode::compare( const QModelIndex & idx1, const QModelIndex & idx2, const QList<SortColumnPair> & columns )
{
	ItemInfo ii1 = rowToItemInfo(idx1.row()), ii2 = rowToItemInfo(idx2.row());
	// To compare 2 rows of the same type, we use the translator to compare
	if( ii1.translatorIndex == ii2.translatorIndex ) {
		void * ptr1 = _itemData(ii1.dataOffset), * ptr2 = _itemData(ii2.dataOffset);
		ModelDataTranslator * trans = _translator(ii1.translatorIndex);
		foreach( SortColumnPair sc, columns ) {
			int result = trans->compare( ptr1, ptr2, idx1.sibling(idx1.row(),sc.first), idx2.sibling(idx2.row(),sc.first), sc.first, sc.second == Qt::AscendingOrder );
			if( result != 0 )
				return (result > 0) ^ (sc.second == Qt::DescendingOrder) ? -1 : 1;
		}
		// Compare pointers if the items are equal in other respects
		bool asc = columns[0].second == Qt::AscendingOrder;
		return ((ptr1 > ptr2) ^ asc) ? -1 : 1;
	}
	// Two rows of different types are compared through the tree builder
	else {
		SuperModel * mdl = model(idx1);
		return mdl->treeBuilder()->compare( idx1, idx2, columns );
	}
	return 0;
}

struct ModelNode::NodeSorter
{
	NodeSorter( ModelNode * node, const QList<SortColumnPair> & columns, const QModelIndex & parent ) : mNode(node), mColumns(columns), mParent(parent) {}
	bool operator()( int row1, int row2 ) {
		int result = mNode->compare( mParent.child( row1, 0 ), mParent.child( row2, 0 ), mColumns );
		return (result > 0);
	}
	ModelNode * mNode;
	const QList<SortColumnPair> & mColumns;
	QModelIndex mParent;
};

void ModelNode::sort ( const QList<SortColumnPair> & columns, bool recursive, const QModelIndex & parent, int start, int end )
{
	if( rowCount() <= 0 ) return;

	start = qMax(0,start);
	if( end == -1 ) end = rowCount() - 1;
	end = qMin(rowCount() - 1,end);
	
	if( end <= start ) return;

	QVector<int> sortVector(rowCount());
	for( int i = rowCount()-1; i>=0; --i )
		sortVector[i] = i;

	// Sort it
	std::stable_sort( 
		&sortVector[start],
		&sortVector[end] + 1,
		NodeSorter( this, columns, parent )
	);

	rearrange( sortVector, parent );
	
	if( recursive ) {
		for( int i=start; i<=end; i++ ) {
			QModelIndex idx = parent.child(i,0);
			if( idx.isValid() && childrenLoaded(idx) ) {
				ModelNode * _child = child(idx);
				if( _child )
					_child->sort( columns, true, idx, 0, -1 );
			}
		}
	}
}

void ModelNode::rearrange( QVector<int> newRowPositions, QModelIndex parent )
{
	int rc = rowCount();
	if( newRowPositions.size() != rc )
		return;

	/* Double check the integrity of the new positions.  The newRowPositions vector
		must contain each row as a source.
	*/
	if(false){
		QVector<bool> sourceCheckVector(rowCount());
		for( int i = rc - 1; i >= 0; i-- )
			sourceCheckVector[i] = false;
		for( int i = rc - 1; i >= 0; i-- ) {
			int source = newRowPositions[i];
			if( sourceCheckVector[source] ) {
				LOG_1( "ERROR! invalid newRowPositions vector!" );
				return;
			}
			sourceCheckVector[source] = true;
		}
	}

	SuperModel * sm = model(parent);
	QVector<ItemInfo> newItemInfoVector(mItemInfoVector.size());
	QModelIndexList from, to;
	QModelIndexList persist = sm->persistentIndexList();
	// Marks which columns we need to report persistent index changes
	int cc = sm->columnCount(sm->index(0,0,parent));
	QBitArray needColumnPIC(cc);
	int pic_first = cc, pic_last = 0;
	foreach( QModelIndex i, persist ) {
		QModelIndex par = i.parent();
		if( par == parent || (!par.isValid() && !parent.isValid()) ) {
			//LOG_1( QString("Row %1, column %2 has persistent index").arg(i.row()).arg(i.column()) );
			int col = i.column();
			if( col >= 0 && !needColumnPIC[col] ) {
				needColumnPIC[col] = true;
				pic_last = qMax(pic_last,col);
				pic_first = qMin(pic_first,col);
			}
			int row = i.row();
			// Use the dataOffset field to indicate that we have persistent indexes to update for this row
			newItemInfoVector[newRowPositions[row]].dataOffset = 1;
		}
	}
	for( int i = rc - 1; i>=0; --i ) {
		int fromIndex = newRowPositions[i];
		if( fromIndex != i ) { //&& newItemInfoVector[i].dataOffset ) {
			for( int c = 0; c <= pic_last; ++c ) {
				if( needColumnPIC[c] ) {
					//LOG_1( QString("Generating index change for row %1, column %2, to row %3").arg(fromIndex).arg(c).arg(i) );
					from.append( sm->nodeToIndex(fromIndex,c,this) );
					to.append( sm->nodeToIndex(i,c,this) );
				}
			}
		}
		newItemInfoVector[i] = mItemInfoVector[fromIndex];
		if( (uint)mChildren.size() > newItemInfoVector[i].index ) {
			ModelNode * child = mChildren[newItemInfoVector[i].index];
			if( child && child != (ModelNode*)1 )
				child->mParentRow = i;
		}
	}
	sm->changePersistentIndexList(from,to);

	mItemInfoVector = newItemInfoVector;
}

void ModelNode::dump(int level)
{
	QString indent;
	for( int i=0; i < level; ++i ) indent += "\t";
	printf( "%sNode %p\n", qPrintable(indent), (void*)this );
	for( int i=0; i<mRowCount; ++i ) {
		ItemInfo & ii = rowToItemInfo(i);
		printf( "%s %i %i\n", qPrintable(indent), i, *(int*)_itemData(ii.dataOffset) );
		if( mChildren.size() > ii.index ) {
			ModelNode * child = mChildren[ii.index];
			if( child && child != (ModelNode*)1 )
				child->dump(level+1);
		}
	}
}

SuperModel::SuperModel( QObject * parent )
: QAbstractItemModel( parent )
, mRootNode( 0 )
, mTreeBuilder( 0 )
, mInsertClosureNode( 0 )
, mBlockInsertNots( false )
, mAutoSort( false )
, mAssumeChildren( false )
, mDisableChildLoading( false )
, mGrouper( 0 )
, mColumnFilterMap()
{
}

SuperModel::~SuperModel()
{
	delete mRootNode;
	delete mGrouper;
}

ModelGrouper * SuperModel::grouper( bool autoCreate )
{
	if( !mGrouper && autoCreate ) {
		mGrouper = new ModelGrouper(this);
		emit grouperChanged( mGrouper );
	}
	return mGrouper;
}

void SuperModel::setGrouper( ModelGrouper * grouper )
{
	if(  mGrouper != grouper ) {
		if( mGrouper ) {
			if( mGrouper->isGrouped() )
				mGrouper->ungroup();
			delete mGrouper;
		}
		mGrouper = grouper;
		emit grouperChanged( mGrouper );
	}
}

ModelTreeBuilder * SuperModel::treeBuilder()
{
	if( !mTreeBuilder ) mTreeBuilder = new ModelTreeBuilder(this);
	return mTreeBuilder;
}

void SuperModel::setTreeBuilder( ModelTreeBuilder * treeBuilder )
{
	mTreeBuilder = treeBuilder;
}

ModelNode * SuperModel::rootNode() const
{
	if( !mRootNode ) {
		mRootNode = new ModelNode(0,0);
		mTreeBuilder->_loadChildren( QModelIndex(), const_cast<SuperModel*>(this), mRootNode );
	}
	return mRootNode;
}

QModelIndex SuperModel::parent( const QModelIndex & idx ) const
{
	if( !idx.isValid() ) return QModelIndex();
	ModelNode * node = indexToNode(idx);
	if( node->mParent ) return createIndex( node->mParentRow, 0, node->mParent );
	return QModelIndex();
}

QModelIndex SuperModel::sibling( int row, int column, const QModelIndex & idx ) const
{
	if( !idx.isValid() || row < 0 ) return QModelIndex();
	ModelNode * node = indexToNode(idx);
	if( row >= node->rowCount() ) return QModelIndex();
	return createIndex( row, column, node );
}

QModelIndex SuperModel::index( int row, int column, const QModelIndex & parent ) const
{
	if( row < 0 || column < 0 ) return QModelIndex();
	ModelNode * childNode = 0;
	bool restoreBlockInsertNots = mBlockInsertNots;
	mBlockInsertNots = true;
	if( parent.isValid() ) {
		ModelNode * node = indexToNode(parent);
		childNode = node->child(parent);
	} else
		childNode = rootNode();
	QModelIndex ret;
	if( childNode && row < childNode->rowCount() )
		ret = createIndex( row, column, childNode );
	mBlockInsertNots = restoreBlockInsertNots;
	return ret;
}

int SuperModel::rowCount( const QModelIndex & parent ) const
{
	if( !parent.isValid() ) return rootNode() ? rootNode()->rowCount() : 0;
	ModelNode * node = indexToNode(parent);
	bool restoreBlockInsertNots = mBlockInsertNots;
	mBlockInsertNots = true;
	ModelNode * childNode = node->child(parent);
	int ret = (childNode ? childNode->rowCount() : 0);
	mBlockInsertNots = restoreBlockInsertNots;
	return ret;
}

bool SuperModel::hasChildren( const QModelIndex & parent ) const
{
	if( !parent.isValid() ) return rootNode()->rowCount() > 0;
	ModelNode * node = indexToNode(parent);
	return node->hasChildren(parent);
}

bool SuperModel::childrenLoaded( const QModelIndex & parent )
{
	if( !parent.isValid() ) return bool(rootNode());
	ModelNode * node = indexToNode(parent);
	return node->childrenLoaded(parent);
}

int SuperModel::columnCount( const QModelIndex & ) const
{
	return mHeaderData.size();
}

QVariant SuperModel::data ( const QModelIndex & index, int role ) const
{
	if( !index.isValid() ) return QVariant();
	QVariant ret = indexToNode(index)->data(index, role);
	// Convert Interval types to strings
	if( ret.userType() == qMetaTypeId<Interval>() )
		return qvariant_cast<Interval>(ret).toDisplayString();
	return ret;
}

bool SuperModel::setData ( const QModelIndex & index, const QVariant & value, int role )
{
	if( !index.isValid() ) return false;
	bool set = indexToNode(index)->setData(index, value, role);
	if( set )
		emit dataChanged(index,index);
	return set;
}

bool SuperModel::dropMimeData( const QMimeData *, Qt::DropAction , int , int, const QModelIndex & )
{
	return false;
}

Qt::ItemFlags SuperModel::flags ( const QModelIndex & index ) const
{
	if( !index.isValid() ) return Qt::ItemFlags(0);
	return indexToNode(index)->flags(index);
}

QModelIndex SuperModel::insert( const QModelIndex & par, int row, ModelDataTranslator * trans )
{
	QList<QModelIndex> ret = insert( par, row, 1, trans );
	return ret.isEmpty() ? QModelIndex() : ret[0];
}

QModelIndexList SuperModel::insert( const QModelIndex & par, int row, int count, ModelDataTranslator * trans, bool skipConstruction )
{
	QModelIndexList ret;
	int maxRow = childrenLoaded(par) ? rowCount(par) : 0;
	if( count < 1 || row > maxRow || row < 0 ) {
		if( mInsertClosureNode && mInsertClosureNode->state == 0 )
			mInsertClosureNode->state = 2;
		return ret;
	}
	ModelNode * node = par.isValid() ? indexToNode(par)->child(par,true) : rootNode();
	if( node ) {
		if( !mBlockInsertNots )
			beginInsertRows( par, row, row + count - 1 );

		if( node->insertChildren( row, count, trans, skipConstruction ) ) {
			for( int i = row; i < row + count; i++ )
				ret += createIndex( i, 0, node );
		}
		if( mInsertClosureNode && mInsertClosureNode->state == 0 ) {
			mInsertClosureNode->state = mBlockInsertNots ? 2 : 1;
			mInsertClosureNode->parent = par;
		} else {
			if( !mBlockInsertNots )
				endInsertRows();
			if( mAutoSort && !skipConstruction ) {
				QList<QPersistentModelIndex> persist;
				foreach( QModelIndex idx, ret ) persist.append( QPersistentModelIndex(idx) );
				ret.clear();
				checkAutoSort(par);
				foreach( QPersistentModelIndex p, persist ) ret.append( p );
			}
		}
	}
	return ret;
}

struct RowSpan {
	RowSpan( const QModelIndex & p, int s, int c ) : parent(p), start(s), count(c) {}
	QPersistentModelIndex parent;
	int start, count;
};

typedef QMap<ModelDataTranslator*,QList<RowSpan> > TransRowSpanMap;

TransRowSpanMap continuousByParentAndTrans( QModelIndexList indexes, SuperModel * sm )
{
	TransRowSpanMap ret;
	typedef QMap<QPair<QModelIndex,ModelDataTranslator*>,QList<int> > BPMap;
	BPMap byParent;
	foreach( QModelIndex idx, indexes )
		byParent[qMakePair(idx.parent(),sm->translator(idx))].append(idx.row());
	for( BPMap::Iterator it = byParent.begin(); it != byParent.end(); ++it ) {
		QList<int> & rows = it.value();
		qSort(rows);
		int lastRow = 0;
		int cnt = 0;
		QModelIndex parent = it.key().first;
		ModelDataTranslator * trans = it.key().second;
		for( int i = rows.size() - 1; i >= 0; --i ) {
			int row = rows[i];
			if( cnt > 0 && row + 1 != lastRow ) {
				ret[trans].append( RowSpan( parent, lastRow, cnt ) );
				cnt = 0;
			}
			lastRow = row;
			cnt++;
		}
		ret[trans].append( RowSpan( parent, lastRow, cnt ) );
	}
	return ret;
}

// Define this to 1 for broken qt beginMoveRows
#define USE_SLOW_PATH 0

#if USE_SLOW_PATH

QModelIndexList SuperModel::move( QModelIndexList indexes, const QModelIndex & destParent, const QModelIndex & insertAt )
{
	ModelNode * destNode = destParent.isValid() ? indexToNode(destParent)->child(destParent,true) : rootNode();
	int insertPos = insertAt.isValid() ? insertAt.row() : rowCount(destParent);
	TransRowSpanMap spansByTrans = continuousByParentAndTrans( indexes, this );
	QModelIndexList ret;
	bool stealObjects = false;
	for( TransRowSpanMap::Iterator it = spansByTrans.begin(); it != spansByTrans.end(); ++it ) {
		int toInsert = 0;
		ModelDataTranslator * trans = it.key();
		QList<RowSpan> & spanList = it.value();
		foreach( const RowSpan & rs, spanList )
			toInsert += rs.count;
//		LOG_5( "Inserting " + QString::number( toInsert ) + " children" );
		beginInsertRows( destParent, insertPos, insertPos + toInsert - 1 );
		if( destNode->insertChildren( insertPos, toInsert, trans, /*skipConstruction=*/false ) ) {
			endInsertRows();
			int currentDestRow = insertPos;
			foreach( const RowSpan & rs, spanList ) {
				ModelNode * sourceNode = rs.parent.isValid() ? indexToNode(rs.parent)->child(rs.parent,true) : rootNode();
//				LOG_5( "Trying beginMoveRows for " + QString::number( rs.count ) + " rows" );
				{
					emit layoutAboutToBeChanged();
					int start = rs.start;
					// If we are moving items without changing parent, we have to adjust the start position if it falls
					// after the insert position
					if( destParent == rs.parent && insertPos < rs.start )
						start += toInsert;
//					LOG_5( "Moving " + QString::number( rs.count ) + " rows starting at " + QString::number(start) );
					// Do the actual copies, we have to do this one at a time because there's no guarantee contiguous
					// items occupy contiguous memory
					for( int i = start + rs.count - 1; i >= start; --i ) {
						QModelIndex destIdx( createIndex( currentDestRow++, 0, destNode ) ), srcIdx( createIndex( i, 0, sourceNode ) );
						if( stealObjects )
							trans->rawCopy( destNode->itemData(destIdx), sourceNode->itemData(srcIdx) );
						else
							trans->copyData( destNode->itemData(destIdx), sourceNode->itemData(srcIdx) );
						// Move the child node pointer, no copying or index updates needed
						destNode->setChild( destIdx, sourceNode->child( srcIdx, /*create=*/false, /*steal=*/true ) );
						for( int c = 0; c < columnCount(); c++ )
							changePersistentIndex( srcIdx.sibling( srcIdx.row(), c ), destIdx.sibling( destIdx.row(), c ) );
					}
					beginRemoveRows( rs.parent, start, start + rs.count - 1 );
					sourceNode->removeChildren( start, rs.count, /*stealObject =*/ stealObjects );
					endRemoveRows();
					if( destParent == rs.parent && insertPos > rs.start ) {
						insertPos -= rs.count;
						currentDestRow -= rs.count;
					}
					emit layoutChanged();
				}
			}
			
			// Since it's possible beginMoveRows failed due to moving onto child or moving onto self
			// we have to reclaim any extra children that weren't used
			if( currentDestRow - insertPos < indexes.size() )
				destNode->removeChildren( currentDestRow, indexes.size() - (currentDestRow - insertPos), /*stealObject =*/ true  );
			
			for( int i = insertPos; i < currentDestRow; i++ )
				ret += createIndex( i, 0, destNode );
		} else
			endInsertRows();
	}
	return ret;
}

#else

QModelIndexList SuperModel::move( QModelIndexList indexes, const QModelIndex & destParent, const QModelIndex & insertAt )
{
	ModelNode * destNode = destParent.isValid() ? indexToNode(destParent)->child(destParent,true) : rootNode();
	QPersistentModelIndex destParentPersist(destParent);
	int insertPos = insertAt.isValid() ? insertAt.row() : rowCount(destParent);
	TransRowSpanMap spansByTrans = continuousByParentAndTrans( indexes, this );
	QModelIndexList ret;
	bool stealObjects = true;
	int currentDestRow = insertPos;
	int totalMoved = 0;
	for( TransRowSpanMap::Iterator it = spansByTrans.begin(); it != spansByTrans.end(); ++it ) {
		ModelDataTranslator * trans = it.key();
		QList<RowSpan> & spanList = it.value();
		foreach( const RowSpan & rs, spanList ) {
			int toInsert = rs.count;
			//LOG_5( "Inserting " + QString::number( toInsert ) + " children" );
			if( beginMoveRows( rs.parent, rs.start, rs.start + rs.count - 1, destParentPersist, currentDestRow ) ) {
				ModelNode * sourceNode = rs.parent.isValid() ? indexToNode(rs.parent)->child(rs.parent,true) : rootNode();
				//LOG_5( "Trying beginMoveRows for " + QString::number( rs.count ) + " rows" );
				if( destNode->insertChildren( currentDestRow, toInsert, trans, /*skipConstruction=*/stealObjects ) ) {
					int start = rs.start;
					// If we are moving items without changing parent, we have to adjust the start position if it falls
					// after the insert position
					if( destParentPersist == rs.parent && insertPos < rs.start )
						start += toInsert;
					//LOG_5( "Moving " + QString::number( rs.count ) + " rows starting at " + QString::number(start) );
					// Do the actual copies, we have to do this one at a time because there's no guarantee contiguous
					// items occupy contiguous memory
					for( int i = start; i < start + rs.count; ++i ) {
						QModelIndex destIdx( createIndex( currentDestRow++, 0, destNode ) ), srcIdx( createIndex( i, 0, sourceNode ) );
						if( stealObjects )
							trans->rawCopy( destNode->itemData(destIdx), sourceNode->itemData(srcIdx) );
						else
							trans->copyData( destNode->itemData(destIdx), sourceNode->itemData(srcIdx) );
						// Move the child node pointer, no copying or index updates needed
						destNode->setChild( destIdx, sourceNode->child( srcIdx, /*create=*/false, /*steal=*/true ) );
					}
					sourceNode->removeChildren( start, rs.count, /*stealObject =*/ stealObjects );
					if( destParentPersist == rs.parent && insertPos > rs.start ) {
						insertPos -= rs.count;
						currentDestRow -= rs.count;
					}
					endMoveRows();
					totalMoved += toInsert;
				} else
					LOG_1( "ModelNode::insertChildren failed" );
			} else
				LOG_1( QString("beginMoveRows failed for source rows %1-%2 moving to %3").arg(rs.start).arg(rs.start+rs.count-1).arg(currentDestRow) );
				
		}
	}
	for( int i = currentDestRow - totalMoved; i < currentDestRow; i++ )
		ret += createIndex( i, 0, destNode );

	if( mAutoSort ) {
		QList<QPersistentModelIndex> persist;
		foreach( QModelIndex idx, ret ) persist.append( QPersistentModelIndex(idx) );
		ret.clear();
		checkAutoSort(destParentPersist);
		foreach( QPersistentModelIndex p, persist ) ret.append( p );
	}
	return ret;
}

#endif

#undef USE_SLOW_PATH

void SuperModel::dump()
{
	mRootNode->dump(0);
}

bool SuperModel::removeRows( int start, int count, const QModelIndex & parent )
{
	if( start < 0 || count < 1 || start + count > rowCount(parent) ) return false;
	ModelNode * node = parent.isValid() ? indexToNode(parent)->child(parent) : rootNode();
	bool ret = false;
	if( node ) {
		beginRemoveRows( parent, start, start + count - 1 );
		ret = node->removeChildren( start, count );
		endRemoveRows();
	}
	return ret;
}

void SuperModel::clear()
{
	removeRows( 0, rowCount() );
}

void SuperModel::remove( const QModelIndex & i )
{
	removeRows( i.row(), 1, i.parent() );
}

/// Groups rows by parent index
/// Then removes from highest row to lowest, calling removeRows
/// for each continuous range.
void SuperModel::remove( QModelIndexList list )
{
	typedef QPair<QPersistentModelIndex,bool> PIV;
	typedef QMap<PIV, QList<int> > QML;
	QML rowsByParent;
	foreach( QModelIndex i, list ) {
		QModelIndex p = i.parent();
		rowsByParent[PIV(p,p.isValid())].append(i.row());
	}
	for( QML::iterator it = rowsByParent.begin(); it != rowsByParent.end(); ++it ) {
		PIV piv = it.key();
		QPersistentModelIndex pmi = piv.first;
		if( pmi.isValid() == piv.second ) {
			QList<int> & rl = it.value();
			qSort(rl);
			int first = -1, last = -1;
			for( int i=rl.size()-1; i>=0; i-- ) {
				int row = rl.at(i);
				if( first == -1 ) { first = last = row; }
				if( row < first - 1 ) {
					removeRows( first, last - first + 1, pmi );
					last = row;
				}
				first = row;
			}
			removeRows( first, last - first + 1, pmi );
		}
	}
}

void SuperModel::swap( const QModelIndex & a, const QModelIndex & b )
{
	bool simpleSwap = (a.parent() == b.parent() && abs(a.row() - b.row()) == 1);
	if( simpleSwap ) {
		ModelNode * node = indexToNode(a);
		node->swap(a,b);
	} else {
		ModelDataTranslator * a_trans = translator(a), * b_trans = translator(b);
		{
			QPersistentModelIndex a_p(a), b_p(b);
			QPersistentModelIndex a_ins = insert(b_p.parent(),b_p.row()+1,b_trans);
			QPersistentModelIndex b_ins = insert(a_p.parent(),a_p.row()+1,a_trans);
			a_trans->copyData(a_trans->dataPtr(a_ins), a_trans->dataPtr(a_p));
			b_trans->copyData(b_trans->dataPtr(b_ins), b_trans->dataPtr(b_p));
		}
		remove(a);
		remove(b);
	}
}

// Inform the model that you have changed some of the data
void SuperModel::dataChange( const QModelIndex & topLeft, const QModelIndex & bottomRight )
{
	if( !topLeft.isValid() ) return;
	QModelIndex br(bottomRight);
	if( !br.isValid() ) br = topLeft;
	emit dataChanged( topLeft, br );
}

void SuperModel::setHeaderLabels( const QStringList & labels )
{
	bool cntChange = (labels.size() != mHeaderData.size());
	mHeaderData = labels;
	if( cntChange )
		reset();
	else
		emit headerDataChanged( Qt::Horizontal, 0, labels.size() );
}

QStringList SuperModel::headerLabels() const
{
	return mHeaderData;
}

QVariant SuperModel::headerData ( int section, Qt::Orientation o, int role ) const
{
	if( role == Qt::DisplayRole ) {
		if( o == Qt::Vertical )
			return "";
		if( section >= 0 && section < mHeaderData.size() )
			return mHeaderData[section];
	}
	return QVariant();
}

void SuperModel::setAutoSort( bool as )
{
	mAutoSort = as;
}

void SuperModel::checkAutoSort( const QModelIndex & parent, bool quiet )
{
	if( mAutoSort )
		_sort( -1, sortOrder(), true, parent, 0, -1, quiet );
}

void SuperModel::resort()
{
	_sort( -1, sortOrder(), true, QModelIndex(), 0, -1, false );
}

void SuperModel::clearChildren( const QModelIndex & i )
{
	if( !i.isValid() ) {
		int rc = rowCount();
		beginRemoveRows(QModelIndex(),0,rc);
		mRootNode->clear();
		endRemoveRows();
		return;
	}
	ModelNode * node = indexToNode(i);
	int rc = node->rowCount();
	beginRemoveRows(i,0,rc);
	node->clearChildren(i);
	endRemoveRows();
}

void SuperModel::sort ( int column, Qt::SortOrder order, bool recursive, const QModelIndex & parentIn, int startRow, int endRow )
{
	_sort( column, order, recursive, parentIn, startRow, endRow, false );
}

void SuperModel::_sort ( int column, Qt::SortOrder order, bool recursive, const QModelIndex & parentIn, int startRow, int endRow, bool quiet )
{
	if( column < 0 )
		column = mSortColumns.isEmpty() ? 0 : mSortColumns[0].first;
	SortColumnPair sc(column,order);
	
	/* Save sort settings for future sorting(resort and auto sort) */
	for( QList<SortColumnPair>::iterator it = mSortColumns.begin(); it != mSortColumns.end(); )
		if( (*it).first == column )
			it = mSortColumns.erase( it );
		else
			++it;
	mSortColumns.prepend(sc);
//	LOG_1( "Sorting with order " + QString( (order == Qt::AscendingOrder) ? "Ascending" : "Descending" ) );
	
	/* Do actual sorting if node is available */
	ModelNode * node = parentIn.isValid() ? indexToNode(parentIn)->child(parentIn) : rootNode();
	if( node ) {
		QModelIndex parent = parentIn.isValid() ? parentIn : createIndex(-1,-1,0);
		if( !quiet )
			emit layoutAboutToBeChanged();
		else
			LOG_1( "Doing quiet sort" );
		node->sort(mSortColumns,recursive,parent,startRow,endRow);
		if( !quiet )
			emit layoutChanged();
	}
}

void SuperModel::moveToTop( QModelIndexList _indexes )
{
	emit layoutAboutToBeChanged();
	QMap< QModelIndex, QModelIndexList > indexesByParent;
	foreach( QModelIndex idx, _indexes )
		if( idx.isValid() )
			indexesByParent[idx.parent()].append(idx);
	for( QMap< QModelIndex, QModelIndexList >::iterator it = indexesByParent.begin(); it != indexesByParent.end(); ++it )
	{
		QModelIndex parent = it.key();
		QModelIndexList indexes = it.value();
		ModelNode * node = parent.isValid() ? indexToNode(parent) : rootNode();
		QVector<int> newPositions(node->rowCount());
		QVector<bool> moved(node->rowCount(),false);
		int pos = 0, lastToCheck = 0;
		foreach( QModelIndex idx, indexes ) {
			if( !moved[idx.row()] ) {
				newPositions[pos++] = idx.row();
				lastToCheck = qMax(lastToCheck, idx.row());
				moved[idx.row()] = true;
			}
		}
		for( int i = 0; i < lastToCheck; ++i )
			if( !moved[i] ) newPositions[pos++] = i;
		for( int i=node->rowCount()-1; i > lastToCheck; i-- )
			newPositions[i] = i;
		node->rearrange( newPositions, parent.isValid() ? parent : createIndex(-1,-1,0) );
	}
	emit layoutChanged();
}

void SuperModel::setSortColumns( const QList<int> & sortColumns, bool forceResort )
{
	mSortColumns.clear();
	foreach( int i, sortColumns )
		mSortColumns.append( SortColumnPair( i, Qt::AscendingOrder ) );
	if( forceResort )
		resort();
	else
		checkAutoSort();
}

void SuperModel::setSortColumns( const QList<SortColumnPair> & sortColumns, bool forceResort )
{
	QList<int> usedColumns;
	mSortColumns = sortColumns;
	for( QList<SortColumnPair>::iterator it = mSortColumns.begin(); it != mSortColumns.end(); ) {
		int col = (*it).first;
		if( col < 0 || col >= columnCount() || usedColumns.contains( col ) ) {
			it = mSortColumns.erase( it );
			continue;
		}
		usedColumns += col;
		++it;
	}
	if( forceResort )
		resort();
	else
		checkAutoSort();
}

Qt::SortOrder SuperModel::sortOrder( int column ) const
{
	if( column < 0 )
		return mSortColumns.isEmpty() ? Qt::DescendingOrder : mSortColumns[0].second;
	return mSortColumns.size() > column ? mSortColumns[column].second : Qt::DescendingOrder;
}

void SuperModel::setSortOrder( Qt::SortOrder order, int column )
{
	if( column < 0 ) column = 0;
	if( column >= mSortColumns.size() ) return;
	mSortColumns[column].second = order;
}

ModelDataTranslator * SuperModel::translator( const QModelIndex & idx ) const
{
	ModelNode * node = indexToNode(idx);
	return node ? node->translator(idx) : rootNode()->translator(idx);
}

SuperModel::InsertClosure::InsertClosure(SuperModel * model)
: mModel( model )
{
	model->openInsertClosure();
}

SuperModel::InsertClosure::~InsertClosure()
{
	mModel->closeInsertClosure();
}

void SuperModel::openInsertClosure()
{
	if( !mInsertClosureNode || mInsertClosureNode->state > 0 ) {
		InsertClosureNode * node = new InsertClosureNode;
		node->count = 0;
		node->state = 0;
		node->next = mInsertClosureNode;
		mInsertClosureNode = node;
	}
	mInsertClosureNode->count++;
}

void SuperModel::closeInsertClosure()
{
	if( mInsertClosureNode ) {
		mInsertClosureNode->count--;
		if( mInsertClosureNode->count == 0 ) {
			if( mInsertClosureNode->state == 1 )
				endInsertRows();
			checkAutoSort(mInsertClosureNode->parent,false);//,true);
			InsertClosureNode * node = mInsertClosureNode;
			mInsertClosureNode = node->next;
			delete node;
		}
	}
}

void SuperModel::setColumnFilter( uint column, const QString & filter )
{
	//LOG_1(QString("setting filter for column %1 to %2").arg(QString::number(column)).arg(filterString));
	mColumnFilterMap[column] = filter;
}
