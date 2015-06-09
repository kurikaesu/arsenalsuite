
#include <qmenu.h>
#include <qpainter.h>

#include "assettemplatedialog.h"
#include "assetdialog.h"
#include "database.h"
#include "elementui.h"
#include "filetrackerdialog.h"
#include "pathtemplatedialog.h"
#include "recorddrag.h"

struct CLASSESUI_EXPORT AssetTemplateItem : public RecordItemBase
{
	Record r;
	bool isRecurseTemplate, isGhostItem;
	QString col1, col2;
	QPixmap icon;
	Qt::ItemFlags _flags;
	void setup( const Record & rec, const QModelIndex & p = QModelIndex() );
	RecordList children( const QModelIndex & );
	QVariant modelData( const QModelIndex & i, int role ) const;
	Qt::ItemFlags modelFlags( const QModelIndex & ) const;
	int compare( const QModelIndex &, const QModelIndex &, int, bool ) const;
	Record getRecord() const { return r; }
	bool setModelData( const QModelIndex &, const QVariant &, int){ return false; }
};

typedef TemplateRecordDataTranslator<AssetTemplateItem> AssetTemplateTranslator;

void AssetTemplateItem::setup( const Record & rec, const QModelIndex & self ) {
	r = rec;
	Element e(r);
	_flags = Qt::ItemFlags(0);
	isRecurseTemplate = false;
	isGhostItem = false;
	QModelIndex parent = self.parent();
	QModelIndex par(parent);
	while( par.isValid() ) {
		Element p(AssetTemplateTranslator::getRecordStatic(par));
		if( p.assetTemplate().isRecord() ) {
			isGhostItem = true;
			break;
		}
		par = par.parent();
	}

	// Set it if we are a recurse template ourselves
	isRecurseTemplate = e.assetTemplate().isRecord();

	if( e.isRecord() ) {
		col1 = e.displayName();
		icon = ElementUi(e).image();
		// If we have an asset template, then overlay the icon
		if( isRecurseTemplate ) {
			QImage over( ":/images/plus_overlay.png" );
			QPainter p( &icon );
			p.drawImage( QPoint(6,6), over );
		}
		if( isGhostItem )
		{
			QPixmap cur(icon.size());
			cur.fill( QColor( 180, 180, 180, 200 ) );
			QPainter p( &icon );
			p.setCompositionMode( QPainter::CompositionMode_SourceAtop );
			p.drawPixmap(QPoint(),cur);
		}
		if( isRecurseTemplate )
			_flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
		else if( isGhostItem )
			_flags = 0;
		else
			_flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
	}
	FileTracker ft(r);
	if( ft.isRecord() ) {
		col1 = ft.name();
		icon = QPixmap( ":/images/tracker.png" );
		_flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
	}
}

QVariant AssetTemplateItem::modelData( const QModelIndex & i, int role ) const
{
	if( role == Qt::DisplayRole && i.column() == 0 )
		return col1;
	if( role == Qt::DisplayRole && i.column() == 1 )
		return col2;
	if( role == Qt::DecorationRole && i.column() == 0 )
		return icon;
	return QVariant();
}

Qt::ItemFlags AssetTemplateItem::modelFlags( const QModelIndex & ) const
{
	return _flags;
}

int AssetTemplateItem::compare( const QModelIndex & idx, const QModelIndex & idx2, int column, bool asc ) const
{
	int i = 0;
	Element e(r), oe(AssetTemplateTranslator::getRecordStatic(idx2));
	if( e.isRecord() ) i++;
	if( oe.isRecord() ) i--;
	if( i==0 )
		return ItemBase::compare(idx,idx2,column,asc);
	return i;
}

RecordList AssetTemplateItem::children( const QModelIndex & parentIndex )
{
	RecordList ret;
	if( AssetTemplateTranslator::isType(parentIndex) ) {
		AssetTemplateItem & ati = AssetTemplateTranslator::data(parentIndex);
		Record r = ati.r;
		Element e(r);
		if( e.isRecord() ) {
			if( e.assetTemplate().isRecord() )
				e = e.assetTemplate().element();
			ret += e.children();
			ret += e.trackers(false/*not recursive*/);
		}
	}
	return ret;
}


AssetTemplateModel::AssetTemplateModel( QObject * parent )
: RecordSuperModel( parent )
{
	new AssetTemplateTranslator(treeBuilder());
	setHeaderLabels( QStringList() << "Name" );
	Table * et = Element::table();
	connect( et, SIGNAL( added( RecordList ) ), SLOT( added( RecordList ) ) );
	connect( et, SIGNAL( removed( RecordList ) ), SLOT( removed( RecordList ) ) );
	connect( et, SIGNAL( updated(  Record , Record ) ), SLOT( updated( Record , Record  ) ) );
	et = FileTracker::table();
	connect( et, SIGNAL( added( RecordList ) ), SLOT( added( RecordList ) ) );
	connect( et, SIGNAL( removed( RecordList ) ), SLOT( removed( RecordList ) ) );
	connect( et, SIGNAL( updated(  Record , Record ) ), SLOT( updated( Record , Record  ) ) );
}

Element AssetTemplateModel::getElement( const QModelIndex & index )
{
	if( !index.isValid() ) return Element();
	return Element(AssetTemplateTranslator::data(index).r);
}

QModelIndex AssetTemplateModel::findElement( const Element & element, bool build )
{
	if( !element.isValid() ) return QModelIndex();
	int cnt = rowCount();
	for( int i=0; i<cnt; i++ ) {
		QModelIndex idx = index(i,0);
		if( idx.isValid() && getElement(idx).key() == element.key() )
			return idx;
	}
	Element p( element.parent() );
	QModelIndex par;
	if( p.isRecord() )
		par = findElement(p);
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

QModelIndex AssetTemplateModel::findFileTracker( const FileTracker & ft, bool build )
{
	Element par = ft.element();
	QModelIndex pari = findElement( par, build );
	if( pari.isValid() ) {
		for( int i = rowCount(pari)-1; i>=0; i-- )
		{
			QModelIndex c = pari.child( i, 0 );
			if( FileTracker(getRecord(c)).key() == ft.key() )
				return c;
		}
	}
	return QModelIndex();
}

void AssetTemplateModel::added( RecordList rl )
{
	foreach( Record r, rl )
	{
		Element e(r);
		if( e.isRecord() ) {
			QModelIndex i = findElement(e);
			// This should be optimized to only re-alloc the block once when adding
			// multiple siblings
			if( !i.isValid() ) {
				i = findElement(e.parent(),false);
				if( i.isValid() )
					AssetTemplateTranslator::data(append( i )).setup(e,i);
			}
		}
		FileTracker ft(r);
		if( ft.isRecord() ) {
			QModelIndex i = findFileTracker(ft);
			if( !i.isValid() ) {
				i = findElement(ft.element(),false);
				if( i.isValid() )
					AssetTemplateTranslator::data(append(i)).setup(ft,i);
			}
		}
	}
}

void AssetTemplateModel::removed( RecordList rl )
{
	foreach( Record r, rl )
	{
		Element e(r);
		if( e.isValid() ) {
			QModelIndex i = findElement(e,false);
			if( i.isValid() )
				remove( i );
		}
		FileTracker ft(r);
		if( ft.isValid() ) {
			QModelIndex i = findFileTracker(ft,false);
			if( i.isValid() )
				remove(i);
		}
	}
}

void AssetTemplateModel::updated( Record r, Record ro )
{
	Element e(r), o(ro);
	if( e.isValid() ) {
		if( e.parent().isRecord() && e.parent() != o.parent() ) {
			removed( o );
			added( e );
			return;
		}
		
		QModelIndex mi = findElement( e );
		if( mi.isValid() ) {
			updateIndex(mi);
			dataChanged(mi,mi);
		}
	}
	FileTracker ft(r), oft(r);
	if( ft.isValid() ) {
		if( ft.element().isRecord() && ft.element() != oft.element() ) {
			removed( ft );
			added( ft );
			return;
		}
		QModelIndex mi = findFileTracker(ft);
		if( mi.isValid() ) {
			updateIndex(mi);
			dataChanged(mi,mi);
		}
	}
}

QStringList AssetTemplateModel::mimeTypes() const
{
	return QStringList() << RecordDrag::mimeType;
}

bool AssetTemplateModel::dropMimeData ( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent )
{
	RecordList rl;
	if( RecordDrag::decode( data, &rl ) ) {
		if( parent.isValid() ) {
			Element e = getElement(parent);
			foreach( Record r, rl ) {
				LOG_5( "AssetTemplateModel::dropMimeData: Dropping record: " + r.table()->tableName() + ":" + QString::number( r.key() ) + " on element: " + QString::number(e.key()) );
				e.dropped( 0, r, action );
			}
			return true;
		}
	}
	return RecordSuperModel::dropMimeData(data,action,row,column,parent);
}

AssetTemplateDialog::AssetTemplateDialog( QWidget * parent )
: QDialog( parent )
{
	setupUi( this );

	AssetTemplateModel * atm = new AssetTemplateModel( mTree );
	mTree->setModel(atm);
	connect( mTree, SIGNAL( showMenu( const QPoint &, const QModelIndex & ) ), SLOT( showContextMenu( const QPoint &, const QModelIndex & ) ) );
	
	mTree->setDragEnabled( true );
	mTree->setAcceptDrops( true );
	mTree->setDropIndicatorShown(true);
}

void AssetTemplateDialog::accept()
{
	if (mTemplateName->text() != mTemplate.name())
	{
		mTemplate.setName( mTemplateName->text() );
		mTemplate.commit();
	}

	QDialog::accept();
}

void AssetTemplateDialog::reject()
{
	QDialog::reject();
}

void AssetTemplateDialog::setTemplate( const AssetTemplate & at )
{
	if( !at.isValid() ) return;
	
	setWindowTitle( at.name() + " Template, " + (at.project().isRecord() ? at.project().name() : QString("None (Global Templates)")) );
	mTemplate = at;
	mTemplateName->setText( at.name() );
	mProject->setText( at.project().isRecord() ? at.project().name() : "None (Global Templates)" );
	mType->setText( at.assetType().name() );
	
	Element e = at.element();
	if( !e.isRecord() ) {
		e = at.assetType().construct();
		e.commit();
		mTemplate.setElement( e );
		mTemplate.commit();
	}
	
	mTree->model()->setRootList( e );
	mTree->expandRecursive();
}

void AssetTemplateDialog::showContextMenu( const QPoint &, const QModelIndex & index )
{
	if( !index.isValid() ) return;
	Record r = AssetTemplateTranslator::getRecordStatic(index);
	bool isRecurseTemplate = AssetTemplateTranslator::data(index).isRecurseTemplate;
	bool isGhostItem = AssetTemplateTranslator::data(index).isGhostItem;

	// Single selection should be on anyway
	if( !r.isRecord() ) return;

	Element e(r);
	FileTracker ft(r);
	
	QMenu * menu = new QMenu( this );

	if( e.isRecord() && !isGhostItem ) {
		QAction * newAsset = 0, * editAsset = 0, * newTracker = 0, * remove = 0;
		newAsset = menu->addAction( "Add Asset" );
		editAsset = menu->addAction( "Edit Asset" );
		newTracker = menu->addAction( "Add File Tracker" );
		remove = menu->addAction( "Remove" );
		if( isRecurseTemplate ) {
			newAsset->setEnabled( false );
			newTracker->setEnabled( false );
		}
		QAction * ret = menu->exec( QCursor::pos() );
		if( ret ) {
			if( ret == newAsset ) {
				Database::current()->beginTransaction( "Create Asset[Template]" );
				AssetDialog * ad = new AssetDialog( e,  this );
				ad->setAssetTemplatesEnabled( true );
				ad->setCreateAssetTemplates( false );
				ad->setPathTemplatesEnabled( true );
				ad->exec();
				Database::current()->commitTransaction();
				delete ad;
			}
			else if( ret == editAsset ) {
				Database::current()->beginTransaction( "Modify Asset[Template]" );
				AssetDialog * ad = new AssetDialog( e,  this );
				ad->setPathTemplatesEnabled( true );
				ad->setAssetTemplatesEnabled( !index.model()->hasChildren(index) );
				ad->setCreateAssetTemplates( false );
				ad->setAsset( e );
				ad->exec();
				Database::current()->commitTransaction();
				delete ad;
			}
			else if( ret == newTracker ) {
				FileTrackerDialog * ftd = new FileTrackerDialog( this );
				if( ftd->exec() == QDialog::Accepted ) {
					FileTracker ft = ftd->fileTracker();
					ft.setElement( e );
					ft.commit();
				}
				delete ftd;
			}
			else if( ret == remove ) {
				e.remove();
			}
		}
	}

	if( ft.isRecord() ) {
		QAction * edit = menu->addAction( "Edit File Tracker" );
		QAction * remove = menu->addAction( "Remove File Tracker" );
		if( isGhostItem ) {
			edit->setEnabled( false );
			remove->setEnabled( false );
		}
		QAction * ret = menu->exec( QCursor::pos() );
		if( ret == edit ) {
			FileTrackerDialog * ftd = new FileTrackerDialog( this );
			ftd->setFileTracker( ft );
			if( ftd->exec() == QDialog::Accepted ) {
				ftd->fileTracker().commit();
			}
			delete ftd;
		} else if( ret == remove ) {
			ft.remove();
		}
	}

	delete menu;
}

