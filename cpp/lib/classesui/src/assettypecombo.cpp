
#include "blurqt.h"
#include "iniconfig.h"

#include "assettypecombo.h"

static bool sColorSort = false;

struct CategoryItem : public RecordItemBase
{
	AssetType at;
	QColor color;
	void setup( const Record & r, const QModelIndex & = QModelIndex() ) { at = r; color.setNamedColor( at.color() ); }
	Record getRecord() { return at; }
	QVariant modelData( const QModelIndex & i, int role );
	Qt::ItemFlags modelFlags( const QModelIndex & );
	int compare( const QModelIndex &, const QModelIndex & idx2, int, bool );
};

typedef TemplateRecordDataTranslator<CategoryItem> CategoryTranslator;

QVariant CategoryItem::modelData( const QModelIndex & i, int role )
{
	if( i.column() == 0 ) {
		if( role == Qt::DisplayRole || role == Qt::EditRole )
			return at.name();
#ifndef Q_OS_WIN // TODO: Re-enable this code when qt gets fixed
		else if( role == Qt::ToolTipRole && !at.description().isEmpty() )
			return "<b>" + at.name() + " Description</b><br/><br/>" + at.description();
#endif // !Q_OS_WIN
		else if( role == Qt::BackgroundColorRole || role == Qt::TextColorRole ) {
			if( color.isValid() ) {
				if( role == Qt::BackgroundColorRole ) return color;
				return QColor( color.value() < 128 ? Qt::white : Qt::black );
			}
		}
	}
	return QVariant();
}

Qt::ItemFlags CategoryItem::modelFlags( const QModelIndex & )
{
	return Qt::ItemFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable );
}

int CategoryItem::compare( const QModelIndex & idx, const QModelIndex & idx2, int column, bool asc )
{
	CategoryItem & other = CategoryTranslator::data(idx2);
	if( sColorSort ) {
		QColor ocol = other.color;
		if( color.hue() != ocol.hue() )
			return color.hue() - ocol.hue();
		if( color.value() != ocol.value() )
			return color.value() - ocol.value();
		if( color.saturation() != ocol.saturation() )
			return color.saturation() - ocol.saturation();
	}
	return ItemBase::compare(idx,idx2,column,asc);
}

AssetTypeCombo::AssetTypeCombo( QWidget * parent )
: RecordCombo( parent )
, mShowDisabled( false )
{
	RecordSuperModel * sm = new RecordSuperModel(this);
	sm->setHeaderLabels(QStringList() << "Name");
	new CategoryTranslator(sm->treeBuilder());
	setModel( sm );
	refresh();
	IniConfig & cfg = userConfig();
	cfg.pushSection( "AssetTypeCombo" );
	sColorSort = cfg.readBool( "SortByColor", true );
	cfg.popSection();
}

void AssetTypeCombo::refresh()
{
	AssetTypeList roles = AssetType::filterByTags( mShowingFirst, mTagFilters ).filter("disabled",0).sorted( "name" );
	AssetTypeList all = AssetType::recordsByTags( mTagFilters ).sorted( "name" ) - roles;
	model()->setRootList( roles + all );
	if( roles.size() )
		model()->sort(0,Qt::DescendingOrder,false,0,roles.size()-1);
	model()->sort(0,Qt::DescendingOrder,false,roles.size(),-1);
	setCurrentIndex( 0 );
}

void AssetTypeCombo::setTagFilters( QStringList tf )
{
	mTagFilters = tf;
	refresh();
}

void AssetTypeCombo::setShowDisabled( bool sd )
{
	if( sd != mShowDisabled ) {
		mShowDisabled = sd;
		refresh();
	}
}

void AssetTypeCombo::setShowFirst( AssetTypeList atl )
{
	mShowingFirst = atl;
	refresh();
}
