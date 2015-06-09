/*
 * BachFolderBrowserTreeView.cpp
 *
 *  Created on: Jun 24, 2009
 *      Author: david.morris
 */

#include "BachFolderBrowserTreeView.h"
#include "BachDirModel.h"
#include "recordtreeview.h"
#include "iniconfig.h"

using namespace Qt;

namespace
{
	static const BachFolderBrowserTreeView::ColumnStructEx DirColumns [] =
	{
		{ "Path", 			"Path",     		"Folder Path", 	150,	BachFolderBrowserTreeView::Column_Path,				false },
		{ "Size", 			"Size",  			"Size", 		35,		BachFolderBrowserTreeView::Column_Size,				true },
		{ "Type", 			"Type",     		"Type", 		35,		BachFolderBrowserTreeView::Column_Type,				true },
		{ "Date", 			"Date", 			"Date", 		35,		BachFolderBrowserTreeView::Column_Date,				true },
		{ "On Disk", 		"OnDisk", 			"Number of Recognised Files On Disk", 
		  55,				BachFolderBrowserTreeView::Column_OnDisk,			false },
		{ "Imported", 		"Imported",  		"Number of Recognised Files Imported into Bach in this Folder ONLY", 
		  65,				BachFolderBrowserTreeView::Column_Imported,			false },
		{ "Imported Total", "ImportedTotal",    "Number of Recognised Files Imported into Bach in this Folder AND ALL FOLDERS", 
		  75,				BachFolderBrowserTreeView::Column_ImportedTotal,	true },
		{ NULL, NULL, NULL, 35, 0, false }
	};
}

//-------------------------------------------------------------------------------------------------
BachFolderBrowserTreeView::BachFolderBrowserTreeView( QWidget * a_Parent )
:	QTreeView( a_Parent )
{
	m_Model = new BachDirModel( QStringList(), QDir::Dirs|QDir::NoDotAndDotDot, QDir::Name, this );
	setModel( m_Model );
	setRootIndex( m_Model->index( "/drd/jobs" ) );

	connect( this, SIGNAL( collapsed(const QModelIndex &) ), SLOT(onCollapsed(const QModelIndex &)));
	connect( this, SIGNAL( expanded(const QModelIndex &) ), SLOT(onExpanded(const QModelIndex &)));
}

//-------------------------------------------------------------------------------------------------
void
BachFolderBrowserTreeView::loadState()
{
	IniConfig & cfg = userConfig();
	cfg.pushSection( "BachFolderBrowserTreeView" );
	setupTreeView( cfg, DirColumns );
	cfg.popSection();
}

//-------------------------------------------------------------------------------------------------
void
BachFolderBrowserTreeView::saveState()
{
	IniConfig & cfg = userConfig();
	cfg.pushSection( "BachFolderBrowserTreeView" );
	saveTreeView( cfg, DirColumns );
	cfg.popSection();
}


//-------------------------------------------------------------------------------------------------
void
BachFolderBrowserTreeView::onCollapsed( const QModelIndex & /*index*/ )
{
}

//-------------------------------------------------------------------------------------------------
void
BachFolderBrowserTreeView::onExpanded( const QModelIndex & /*index*/ )
{
}

//-------------------------------------------------------------------------------------------------
QString
BachFolderBrowserTreeView::getSelectedPath() const
{
	QModelIndexList list = selectedIndexes();
	if ( list.isEmpty() )
		return QString();

	return m_Model->filePath( list[ 0 ] );
}

//-------------------------------------------------------------------------------------------------
void
BachFolderBrowserTreeView::forgetPath( const QString & a_Path )
{
	m_Model->forgetPath( a_Path );
}

//-------------------------------------------------------------------------------------------------
void
BachFolderBrowserTreeView::showTotals( bool a_ShowTotals )
{
	m_Model->showTotals( a_ShowTotals );
	header()->setSectionHidden( Column_ImportedTotal, !a_ShowTotals );
	header()->update();
	update();
}

//-------------------------------------------------------------------------------------------------
void
BachFolderBrowserTreeView::setupColumns( IniConfig & ini, const BachFolderBrowserTreeView::ColumnStructEx columns [] )
{
	QHeaderView * hdr = header();
	int cnt = 0;
	QStringList labels;
	for( cnt=0; columns[cnt].name; ++cnt );
	QVector<int> indexVec(cnt);
	for( int i=0; i<cnt; i++ ) {
		labels << QString::fromLatin1(columns[i].name);
		indexVec[i] = ini.readInt( columns[i].iniName + QString("Index"), columns[i].defaultPos );
	}
	hdr->setStretchLastSection(false);
	for( int n=0; n<cnt; n++ ) {
		for( int i=0; i<cnt; i++ )
			if( indexVec[i] == n )
				hdr->moveSection( hdr->visualIndex(i), n );
	}
	hdr->resizeSections(QHeaderView::Stretch);
	for( int n=0; n<cnt; n++ ) {
		int size = ini.readInt( columns[n].iniName + QString("Size"), columns[n].defaultSize );
		hdr->resizeSection( n, size==0?columns[n].defaultSize:size );
	}
	for( int n=0; n<cnt; n++ ) {
		bool hidden = ini.readBool( columns[n].iniName + QString("Hidden"), columns[n].defaultHidden );
		hdr->setSectionHidden( n, hidden );
	}
	hdr->setResizeMode( QHeaderView::Interactive );
}

//-------------------------------------------------------------------------------------------------
void
BachFolderBrowserTreeView::saveColumns( IniConfig & ini, const BachFolderBrowserTreeView::ColumnStructEx columns [] )
{
	QHeaderView * hdr = header();
	for( int i=0; columns[i].name; i++ )
		ini.writeInt( columns[i].iniName + QString("Size"), hdr->sectionSize( i ) );
	for( int i=0; columns[i].name; i++ )
		ini.writeInt( columns[i].iniName + QString("Index"), hdr->visualIndex( i ) );
	for( int i=0; columns[i].name; i++ )
		ini.writeBool( columns[i].iniName + QString("Hidden"), hdr->isSectionHidden( i ) );
}

//-------------------------------------------------------------------------------------------------
void
BachFolderBrowserTreeView::setupTreeView( IniConfig & ini, const BachFolderBrowserTreeView::ColumnStructEx columns [] )
{
	setupColumns( ini, columns );
	int sc = ini.readInt("SortColumn", 0);
	Qt::SortOrder order(Qt::SortOrder(ini.readInt("SortOrder",Qt::AscendingOrder)));
	header()->setSortIndicator(sc,order);
	model()->sort(sc,order);
}

//-------------------------------------------------------------------------------------------------
void
BachFolderBrowserTreeView::saveTreeView( IniConfig & ini, const BachFolderBrowserTreeView::ColumnStructEx columns [] )
{
	saveColumns( ini, columns );
	ini.writeInt( "SortColumn", header()->sortIndicatorSection() );
	ini.writeInt( "SortOrder", header()->sortIndicatorOrder() );
}

//-------------------------------------------------------------------------------------------------
QString 
BachFolderBrowserTreeView::getTitle( Columns a_Which )
{
	for ( int i=0; DirColumns[i].name; i++ )
		if ( DirColumns[ i ].defaultPos == a_Which )
			return QString( DirColumns[i].name );
	return QString( "<not found>" );
}

//-------------------------------------------------------------------------------------------------
QString 
BachFolderBrowserTreeView::getToolTip( Columns a_Which )
{
	for ( int i=0; DirColumns[i].name; i++ )
		if ( DirColumns[ i ].defaultPos == a_Which )
			return QString( DirColumns[i].toolTip );
	return QString( "<not found>" );
}


