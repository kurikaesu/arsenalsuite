
#include "assettype.h"
#include "elementui.h"
#include "elementmodel.h"
#include "user.h"
#include "project.h"
#include "projectstatus.h"
#include "blurqt.h"
#include "elementstatus.h"
#include "qvariantcmp.h"
#include "recorddrag.h"

struct CLASSESUI_EXPORT ElementModelItem : public RecordItemBase
{
	Element element;
	QString col1, col2;
	QPixmap icon;
	QColor statusColor;
	ElementModelItem( const Element & e  ) { setup(e); }
	ElementModelItem(){}
	void setup( const Record & el, const QModelIndex & = QModelIndex() );
	RecordList children( const QModelIndex & );
	QVariant modelData( const QModelIndex &, int role ) const;
	Qt::ItemFlags modelFlags( const QModelIndex & ) const;
	int compare( const QModelIndex &, const QModelIndex &, int, bool ) const;
	Record getRecord() const { return element; }
	bool setModelData( const QModelIndex &, const QVariant &, int){ return false; }
};

typedef TemplateRecordDataTranslator<ElementModelItem> ElementTranslator;

void ElementModelItem::setup( const Record & el, const QModelIndex & index )
{
	ElementModel * em = (ElementModel*)index.model();
	element = el;
	col1 = element.displayName(em && em->namesNeedContext());
	icon = ElementUi(element).image();
	if( em && em->secondColumnIsLocation() ) {
		Element par = element.parent();
		if( !par.isRecord() ) par = element;
		col2 = par.displayName(true);
		statusColor = Qt::white;
	} else {
		Project p(element);
		if( p.isRecord() ) {
			statusColor = Qt::white;
			col2 = p.projectStatus().projectStatus();
		} else {
			statusColor.setNamedColor( element.elementStatus().color() );
			col2 = element.elementStatus().name();
		}
	}
}

QVariant ElementModelItem::modelData ( const QModelIndex & i, int role ) const
{
	if( role == Qt::DecorationRole && i.column() == 0 )
		return icon;
	if( role == Qt::DisplayRole || role == Qt::EditRole ) {
		switch( i.column() ) {
			case 0: return col1;
			case 1: return col2;
		}
	}
	if( !((ElementModel*)i.model())->secondColumnIsLocation() ) {
		if( i.column() == 1 && role == Qt::BackgroundColorRole )
			return statusColor;
		else if( i.column() == 1 && role == Qt::TextColorRole )
			return QColor(statusColor.value() < 128 ? Qt::white : Qt::black);
	}
	return QVariant();
}

Qt::ItemFlags ElementModelItem::modelFlags ( const QModelIndex & ) const
{
	return Qt::ItemFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
}

int ElementModelItem::compare( const QModelIndex & i, const QModelIndex & idx2, int column, bool asc ) const
{
	ElementModelItem & other = ElementTranslator::data(idx2);
	if( column == 0 ) {
		AssetType at = element.assetType(), oat = other.element.assetType();
		if( at != oat )
			return at.sortNumber() - oat.sortNumber();
		QString sc = at.sortColumn();
		if( sc != "displayName" ) {
			QVariant v = element.getValue( at.sortColumn() ), ov = other.element.getValue( at.sortColumn() );
			return qVariantCmp( v, ov );
		}
		return compareRetI(col1,other.col1);
	}else if( i.column() == 1 )
		return compareRetI(col2, other.col2);
	return element.key() - other.element.key();
}

RecordList ElementModelItem::children( const QModelIndex & parent )
{
	RecordList children;
	if( !User(element).isRecord() )
		children = element.children();
	LOG_5( "here" );
	return children;
}

ElementModel::ElementModel( QObject * parent )
: RecordSuperModel( parent )
, mNamesNeedContext( false )
, mSecondColumnIsLocation( false )
{
	new ElementTranslator(treeBuilder());
	QStringList hl;
	hl << "Name" << "Status";
	setHeaderLabels( hl );
	Table * et = Element::table();
	connect( et, SIGNAL( added( RecordList ) ), SLOT( added( RecordList ) ) );
	connect( et, SIGNAL( removed( RecordList ) ), SLOT( removed( RecordList ) ) );
	connect( et, SIGNAL( updated(  Record , Record ) ), SLOT( updated( Record , Record  ) ) );
}

ElementModel::~ElementModel()
{
}

Element ElementModel::getElement( const QModelIndex & index )
{
	return ElementTranslator::isType(index) ? ElementTranslator::data(index).element : Element();
}

void ElementModel::setElementList( ElementList list )
{
	setRootList( list );
}

ElementList ElementModel::elementList() const
{
	return rootList();
}

QModelIndex ElementModel::findElement( const Element & element, bool build )
{
	if( !element.isValid() ) return QModelIndex();
	int cnt = rowCount();
	for( int i=0; i<cnt; i++ ) {
		QModelIndex idx = index(i,0);
		if( idx.isValid() && getElement(idx).key() == element.key() )
			return idx;
	}
	Element p = element.parent();
	QModelIndex par = p.isRecord() ? findElement(p) : QModelIndex();
	if( !par.isValid() ) return par;
	if( !build && !childrenLoaded(par) ) return QModelIndex();
	for( int i = rowCount(par)-1; i>=0; i-- )
	{
		QModelIndex c = par.child( i, 0 );
		if( getElement(c).key() == element.key() )
			return c;
	}
	return QModelIndex();
}

void ElementModel::added( RecordList rl )
{
	foreach( Element e, rl )
	{
		if( !e.isValid() ) continue;
		
		LOG_5( "ElementModel::added: key: " + QString::number(e.key()) + " parent:" + QString::number(e.parent().key()) + " name: " + e.displayName() );
		QModelIndex i = findElement(e);
		// This should be optimized to only re-alloc the block once when adding
		// multiple siblings
		if( !i.isValid() ) {
			i = findElement(e.parent(),false);
			if( i.isValid() )
				QModelIndex a = append(e,i);
		}
	}
}

void ElementModel::removed( RecordList rl )
{
	LOG_3( "ElementModel::removed:" );
	foreach( Element e, rl )
	{
		if( !e.isValid() ) continue;
		LOG_5( "ElementModel::removed" );
		QModelIndex i = findElement(e,false);
		if( i.isValid() ) {
			LOG_3( "ElementModel::removed: Calling remove( QModelIndex )" );
			remove( i );
		}
		else LOG_5( "Couldn't find element: " + e.displayName() + " for removal" );
	}
}

void ElementModel::updated( Record r, Record ro )
{
	Element e(r), o(ro);
	LOG_5( "ElementModel::updated" );
	if( e.parent() != o.parent() ) {
		if( o.parent().isRecord() )
			removed( o );
		if( e.parent().isRecord() )
			added( e );
		return;
	}
	
	QModelIndex mi = findElement( e );
	if( mi.isValid() ) {
		updateIndex(mi);
		dataChanged(mi,mi);
	}
}

QStringList ElementModel::mimeTypes() const
{
	return QStringList() << RecordDrag::mimeType;
}

bool ElementModel::dropMimeData ( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent )
{
	RecordList rl;
	if( RecordDrag::decode( data, &rl ) ) {
		if( parent.isValid() ) {
			Element e = getElement(parent);
			foreach( Record r, rl )
				e.dropped( 0, r, action );
			return true;
		}
	}
	return RecordSuperModel::dropMimeData(data,action,row,column,parent);
}

void ElementModel::setNamesNeedContext( bool nnc )
{
	mNamesNeedContext = nnc;
}

void ElementModel::setSecondColumnIsLocation( bool scil )
{
	mSecondColumnIsLocation = scil;
	setHeaderLabels( QStringList() << "Name" << "Location" );
}
