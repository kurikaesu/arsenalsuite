

#ifndef RECORD_SUPER_MODEL_H
#define RECORD_SUPER_MODEL_H

#include "stonegui.h"
#include "record.h"
#include "recordlist.h"
#include "tableschema.h"

#include "supermodel.h"

namespace Stone {
	class Table;
};
using Stone::Table;

class RecordItemBase;
/**
 * Abstract class that provides generic methods dealing with Records
 * Can get a record per index with getRecord, can setup an index with
 * a record using setup.
 * Can add indexes that are setup with a records using insertRecordList,
 * and appendRecordList.
 *
 * This class should generally be used with TemplateRecordDataTranslator
 * for custom Item classes, or with BasicRecordDataTranslator.
 */
class STONEGUI_EXPORT RecordDataTranslatorInterface : public ModelDataTranslator
{
public:
	RecordDataTranslatorInterface(ModelTreeBuilder * builder) : ModelDataTranslator(builder){}

	virtual QVariant recordData(const Record &, const QModelIndex &, int role) const = 0;
	virtual bool setRecordData(Record, const QModelIndex &, const QVariant &, int role) = 0;
	virtual Qt::ItemFlags recordFlags(const Record &, const QModelIndex &) const = 0;
	virtual int recordCompare( const Record &, const Record &, const QModelIndex &, const QModelIndex &, int, bool ) const = 0;
	virtual RecordList recordChildren( const Record &, const QModelIndex & ) const = 0;
	static const char * IfaceName;
	virtual const void * iface( const char * iface ) const;
	static const RecordDataTranslatorInterface * cast( const ModelDataTranslator * );

	virtual Record getRecord(const QModelIndex &) const = 0;
	virtual RecordList children(const QModelIndex &) const = 0;
	virtual void setup(const QModelIndex & idx, const Record & record ) = 0;
	virtual QModelIndexList insertRecordList(int row, RecordList & rl, const QModelIndex & parent = QModelIndex() ) = 0;
	virtual QModelIndexList appendRecordList(RecordList rl, const QModelIndex & parent = QModelIndex() );
};

/**
 * Maps model columns to Record fields, with support for setting editable flags and
 * showing foreign key values.
 */
class STONEGUI_EXPORT RecordDataTranslatorBase : public RecordDataTranslatorInterface
{
public:
	RecordDataTranslatorBase(ModelTreeBuilder * builder) : RecordDataTranslatorInterface(builder){}
	~RecordDataTranslatorBase() {}
	
	virtual QVariant recordData(const Record &, const QModelIndex &, int role) const;
	virtual bool setRecordData(Record, const QModelIndex &, const QVariant &, int role);
	virtual Qt::ItemFlags recordFlags(const Record &, const QModelIndex &) const;
	virtual int recordCompare( const Record &, const Record &, const QModelIndex &, const QModelIndex &, int, bool ) const;
	virtual RecordList recordChildren( const Record &, const QModelIndex & ) const { return RecordList(); }

	void setRecordColumnList( QStringList columns, bool defaultEditable = false );
	void setColumnEditable( int column, bool editable );
	QString recordColumnName( int column ) const;
	int recordColumnPos( int column, const Record & ) const;
	bool columnIsEditable( int column ) const;
	TableSchema * columnForeignKeyTable( int column ) const;
	
protected:
	virtual void _virt(){}

	struct ColumnEntry {
		QString columnName;
		int columnPos;
		bool editable;
		TableSchema * foreignKeyTable;
	};

	// These map the column to the field to show from the record
	mutable QVector<ColumnEntry> mColumnEntries;
};

class STONEGUI_EXPORT SipRecordDataTranslatorBase : public RecordDataTranslatorBase
{
public:
	SipRecordDataTranslatorBase(ModelTreeBuilder * builder) : RecordDataTranslatorBase(builder){}
	~SipRecordDataTranslatorBase() {}

	virtual QVariant recordData(const Record & r, const QModelIndex & idx, int role) const { return RecordDataTranslatorBase::recordData(r, idx, role); }

	virtual Record getRecord(const QModelIndex & ) const { return Record(); }
	virtual RecordList children(const QModelIndex &) const { return RecordList(); }
	virtual void setup(const QModelIndex &, const Record & ) {}
	virtual QModelIndexList insertRecordList(int , RecordList & , const QModelIndex & = QModelIndex() ) { return QModelIndexList(); }

	virtual int dataSize() { return 0; }
	virtual QVariant modelData( void *, const QModelIndex &, int ) const { return QVariant(); }
	virtual bool setModelData( void *, const QModelIndex &, const QVariant &, int ) { return false; }
	virtual Qt::ItemFlags modelFlags( void *, const QModelIndex & ) const { return 0; }
	virtual int compare( void *, void *, const QModelIndex &, const QModelIndex &, int, bool ) const { return 0; }
	virtual void deleteData( void * ) {}
	virtual void constructData( void *, void * = 0 ) {}
	virtual void copyData( void *, void * ) {}
};

/**
 * Template class for setting up a translator that implements the RecordDataTranslator
 * interface.  Custom item types must provide the following functions
 * 		void setup( const Record &, const QModelIndex & ) const
 * 		Record getRecord() const
 */
template<class TYPE, class ROOT_BASE = RecordDataTranslatorBase> class TemplateRecordDataTranslator : public TemplateDataTranslator<TYPE,ROOT_BASE>
{
public:
	typedef TemplateDataTranslator<TYPE,ROOT_BASE> BASE;
	TemplateRecordDataTranslator(ModelTreeBuilder * builder) : BASE(builder) {}
	
 	QVariant modelData( void * dataPtr, const QModelIndex & idx, int role ) const {
		QVariant ret = BASE::modelData(dataPtr,idx,role);
		if( !ret.isValid() )
			return this->recordData(getRecord(idx),idx,role);
		return ret;
	}

	bool setModelData( void * dataPtr, const QModelIndex & idx, const QVariant & value, int role ) {
		bool ret = BASE::setModelData(dataPtr,idx,value,role);
		if( !ret )
			return this->setRecordData(getRecord(idx),idx,value,role);
		return ret;
	}

	Qt::ItemFlags modelFlags( void * dataPtr, const QModelIndex & idx ) const
	{
		Qt::ItemFlags ret = BASE::modelFlags(dataPtr,idx);
		if( ret == 0 ) return this->recordFlags(getRecord(idx),idx);
		return ret;
	}

	int compare( void * dataPtr, void * dataPtr2, const QModelIndex & idx1, const QModelIndex & idx2, int column, bool asc ) const
	{
		int ret = BASE::compare(dataPtr,dataPtr2,idx1,idx2,column,asc);
		if( ret == 0 ) return this->recordCompare(getRecord(idx1),getRecord(idx2),idx1,idx2,column,asc);
		return ret;
	}

//	virtual bool setModelData( TYPE * data, const QModelIndex & idx, const QVariant & value, int role ) const;
//	virtual Qt::ItemFlags modelFlags( TYPE * data, const QModelIndex & idx ) const;

	static Record getRecordStatic(const QModelIndex &idx) { return BASE::data(idx).getRecord(); }
	static RecordList getChildrenStatic(const QModelIndex & idx) { return BASE::data(idx).children(idx); }
	virtual Record getRecord(const QModelIndex &idx) const { return getRecordStatic(idx); }
	virtual RecordList children(const QModelIndex &idx) const {
		RecordList ret = getChildrenStatic(idx);
		if( ret.isEmpty() )
			return this->recordChildren(getRecord(idx),idx);
		return ret;
	}
	virtual void setup(const QModelIndex & idx, const Record & record ) { BASE::data(idx).setup(record,idx); }
	virtual QModelIndexList insertRecordList( int row, RecordList & rl, const QModelIndex & parent ) {
		if( rl.size() <= 0 ) return QModelIndexList();
		SuperModel * model = BASE::model();
		SuperModel::InsertClosure closure(model);
		QModelIndexList ret = model->insert( parent, row, rl.size(), this );
		for( int i=0; i < (int)rl.size(); ++i )
			setup(ret[i],rl[i]);
		return ret;
	}
};

class STONEGUI_EXPORT RecordItemBase : public ItemBase
{
public:
	RecordList children( const QModelIndex & ) { return RecordList(); }
	Qt::ItemFlags modelFlags( const QModelIndex & ) { return Qt::ItemFlags(0); }
};

class STONEGUI_EXPORT SipRecordItemBase : public RecordItemBase
{
public:
	SipRecordItemBase(ModelTreeBuilder *) {}
};

/**
 * Basic Record Item class.  This class provides default implementations of
 * operator==
 * setup
 * recordData
 * setRecordData
 * and flags.
 * This class needs to be used with a RecordDataTranslator subclass
 * that provides a column list and editable field flags.
 */

class STONEGUI_EXPORT RecordItem : public RecordItemBase
{
public:
	Record record;
	Record getRecord() const { return record; }
	void setup( const Record & r, const QModelIndex & );
};

typedef TemplateRecordDataTranslator<RecordItem,RecordDataTranslatorBase> RecordDataTranslator;


class STONEGUI_EXPORT RecordTreeBuilder : public ModelTreeBuilder
{
public:
	RecordTreeBuilder( SuperModel * model ) : ModelTreeBuilder( model ) {}

	virtual bool hasChildren( const QModelIndex & parentIndex, SuperModel * model );
	virtual void loadChildren( const QModelIndex & parentIndex, SuperModel * model );
};

/**
 * Provides a subclass of model with convenience records for dealing with
 * Records directly.  Interfaces the indexes through the RecordDataTranslatorInterface
 * in order to get/set record data.
 */
class STONEGUI_EXPORT RecordSuperModel : public SuperModel
{
Q_OBJECT
public:
	RecordSuperModel( QObject * parent );

	RecordList rootList() const;

	Record getRecord(const QModelIndex & i) const;
	RecordList getRecords( const QModelIndexList & list ) const;

	void updateIndex( const QModelIndex & i );

	bool setupIndex( const QModelIndex & i, const Record & r );

	QModelIndex findIndex( const Record & r, bool recursive = true, const QModelIndex & parent = QModelIndex(), bool loadChildren = true );
	QModelIndexList findIndexes( RecordList, bool recursive = true, const QModelIndex & parent = QModelIndex(), bool loadChildren = true );
	QModelIndex findFirstIndex( RecordList, bool recursive = true, const QModelIndex & parent = QModelIndex(), bool loadChildren = true );

	RecordList listFromIS( const QItemSelection & is );

	QMimeData * mimeData(const QModelIndexList &indexes) const;
	QStringList mimeTypes() const;
	virtual bool dropMimeData ( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent );
	
	virtual void setupChildren( const QModelIndex & parent, const RecordList & rl );

	void listen( Table * t );

	RecordDataTranslatorInterface * recordDataTranslator( const QModelIndex & i ) const;

	using SuperModel::insert;
	using SuperModel::append;
	using SuperModel::remove;
public slots:

	virtual QModelIndexList insert( RecordList rl, int row = 0, const QModelIndex & parent = QModelIndex(), RecordDataTranslatorInterface * trans = 0 );
	virtual QModelIndex append( const Record &, const QModelIndex & parent = QModelIndex(), RecordDataTranslatorInterface * trans = 0 );
	virtual QModelIndexList append( RecordList rl, const QModelIndex & parent = QModelIndex(), RecordDataTranslatorInterface * trans = 0 );
	virtual void remove( RecordList rl, bool recursive = false, const QModelIndex & parent = QModelIndex() );
	
	virtual void updated( RecordList rl, bool recursive = false, const QModelIndex & parent = QModelIndex(), bool loadChildren = true );
	virtual void updated( Record r) { updated( RecordList(r) ); }

	virtual void setRootList( const RecordList & );

	virtual void updateRecords( RecordList, const QModelIndex & parent = QModelIndex(), bool updateRecursive = false, bool loadChildren = true );

protected:
	QModelIndexList findIndexesHelper( RecordList rl, bool recursive, const QModelIndex & index, bool retAfterOne = false, bool loadChildren = true );
};

#endif // RECORD_SUPER_MODEL_H
