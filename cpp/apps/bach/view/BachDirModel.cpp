/*
 * Copyright (c) 2009 Dr. D Studios. (Please refer to license for details)
 * SVN_META_HEADURL = "$HeadURL: $"
 * SVN_META_ID = "$Id: BachDirModel.cpp 9408 2010-03-03 22:35:49Z brobison $"
 */

#include "BachDirModel.h"
#include "BachFolderBrowserTreeView.h"
#include "qdebug.h"
#include <database.h>
#include <qapplication.h>

//-------------------------------------------------------------------------------------------------
BachDirModel::BachDirModel( const QStringList & a_NameFilters,
							QDir::Filters a_Filters,
							QDir::SortFlags a_Sort,
							QObject * a_Parent )
:	QDirModel( a_NameFilters, a_Filters, a_Sort, a_Parent )
,	m_BaseColumn( 4 )
,	m_ShowTotals( false )
{
}

//-------------------------------------------------------------------------------------------------
BachDirModel::~BachDirModel()
{

}

//-------------------------------------------------------------------------------------------------
QVariant
BachDirModel::data( const QModelIndex & a_Idx, int a_Role ) const
{
	QVariant var;
	if ( a_Idx.column() < m_BaseColumn )
		var = QDirModel::data( a_Idx, a_Role );

	int col = a_Idx.column();
	if ( col == BachFolderBrowserTreeView::Column_OnDisk )
	{
		if ( a_Role == Qt::DisplayRole )
		{
			QString path = filePath( a_Idx );
			BachDirModel::Counts counts = getCounts( path );
			return QVariant( counts.real );
		}
		else
		{
			// qWarning() << "Real" << a_Idx << a_Role << var;
		}
	}
	else if ( col == BachFolderBrowserTreeView::Column_Imported )
	{
		if ( a_Role == Qt::DisplayRole )
		{
			QString path = filePath( a_Idx );
			BachDirModel::Counts counts = getCounts( path );
			return QVariant( counts.partial );
		}
		else
		{
			// qWarning() << "Partial" << a_Idx << a_Role << var;
		}
	}
	else if ( col == BachFolderBrowserTreeView::Column_ImportedTotal && m_ShowTotals )
	{
		if ( a_Role == Qt::DisplayRole )
		{
			QString path = filePath( a_Idx );
			BachDirModel::Counts counts = getCounts( path );
			return QVariant( counts.total );
		}
		else
		{
			// qWarning() << "Total" << a_Idx << a_Role << var;
		}
	}

	return var;
}

//-------------------------------------------------------------------------------------------------
QVariant
BachDirModel::headerData ( int a_Section, Qt::Orientation a_Orientation, int a_Role ) const
{

	if ( a_Orientation != Qt::Horizontal )
		return QDirModel::headerData( a_Section, a_Orientation, a_Role );

	if ( a_Role == Qt::DisplayRole )
	{
		if ( a_Section == BachFolderBrowserTreeView::Column_OnDisk )
		{
			return QVariant( BachFolderBrowserTreeView::getTitle( BachFolderBrowserTreeView::Column_OnDisk ) );
		}
		else if ( a_Section == BachFolderBrowserTreeView::Column_Imported )
		{
			return QVariant( BachFolderBrowserTreeView::getTitle( BachFolderBrowserTreeView::Column_Imported ) );
		}
		else if ( a_Section == BachFolderBrowserTreeView::Column_ImportedTotal )
		{
			return QVariant( BachFolderBrowserTreeView::getTitle( BachFolderBrowserTreeView::Column_ImportedTotal ) );
		}
	}
	else if ( a_Role == Qt::ToolTipRole )
	{
		if ( a_Section == BachFolderBrowserTreeView::Column_OnDisk )
		{
			return QVariant( BachFolderBrowserTreeView::getToolTip( BachFolderBrowserTreeView::Column_OnDisk ) );
		}
		else if ( a_Section == BachFolderBrowserTreeView::Column_Imported )
		{
			return QVariant( BachFolderBrowserTreeView::getToolTip( BachFolderBrowserTreeView::Column_Imported ) );
		}
		else if ( a_Section == BachFolderBrowserTreeView::Column_ImportedTotal )
		{
			return QVariant( BachFolderBrowserTreeView::getToolTip( BachFolderBrowserTreeView::Column_ImportedTotal ) );
		}
	}
	return QDirModel::headerData( a_Section, a_Orientation, a_Role );
}


//-------------------------------------------------------------------------------------------------
int
BachDirModel::columnCount( const QModelIndex & /*a_Parent*/ ) const
{
	return BachFolderBrowserTreeView::Column_Last;
}

//-------------------------------------------------------------------------------------------------
BachDirModel::Counts
BachDirModel::getCounts( const QString & a_Path ) const
{
	QHash< QString, Counts >::const_iterator it = m_CachedCounts.constFind( a_Path );
	if ( it != m_CachedCounts.constEnd() )
	{
		return *it;
	}

	qApp->setOverrideCursor(QCursor(Qt::WaitCursor));

	QString queryPath = a_Path;
	if ( !queryPath.endsWith( '/' ) )
		queryPath.append( '/' );

	BachDirModel::Counts counts;
	counts.total = -1;
	QSqlQuery query;
	if ( m_ShowTotals )
	{
		query = Database::current()->exec( "SELECT COUNT(*) FROM ONLY BachAsset WHERE directory like '"+queryPath+"%'",	VarList() );
		if ( query.next() )
		{
			counts.total = query.value( 0 ).toInt();
		}
	}
	query = Database::current()->exec( "SELECT COUNT(*) FROM ONLY BachAsset WHERE directory = '"+queryPath+"'",	VarList() );
	if ( query.next() )
	{
		counts.partial = query.value( 0 ).toInt();
	}
	QDir dir ( a_Path, "*.png *.exr *.sgi *.jpg *.jpeg *.raw *.gif *.mov *.avi *.mpg *.mpeg *.tga *.tiff *.tif *.mp4 *.cr2 *.psd", QDir::NoSort, QDir::Files );
	counts.real = dir.count();

	m_CachedCounts[ a_Path ] = counts;

	qApp->restoreOverrideCursor();

	return counts;
}

//-------------------------------------------------------------------------------------------------
void
BachDirModel::forgetPath( const QString & a_Path )
{
	QHash< QString, Counts >::iterator it = m_CachedCounts.find( a_Path );
	if ( it != m_CachedCounts.end() )
	{
		m_CachedCounts.erase( it );
	}
}

//-------------------------------------------------------------------------------------------------
void
BachDirModel::showTotals( bool a_ShowTotals )
{
	m_CachedCounts.clear();
	m_ShowTotals = a_ShowTotals;
}

