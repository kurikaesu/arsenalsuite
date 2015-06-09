/*
 * Copyright (c) 2009 Dr. D Studios. (Please refer to license for details)
 * SVN_META_HEADURL = "$HeadURL: $"
 * SVN_META_ID = "$Id: BachDirModel.h 9408 2010-03-03 22:35:49Z brobison $"
 */

#ifndef _DRD_BACHDIRMODEL_H_
#define _DRD_BACHDIRMODEL_H_

#include <QDirModel>
#include <QHash>
#include "utils.h"

//---------------------------------------------------------------------------------------------
//
class BachDirModel : public QDirModel
{
public:
	BachDirModel( const QStringList & a_NameFilters, QDir::Filters a_Filters, QDir::SortFlags a_Sort, QObject * a_Parent );
	virtual ~BachDirModel();

	QVariant data( const QModelIndex & a_Idx, int a_Role = Qt::DisplayRole ) const;
	virtual QVariant headerData ( int a_Section, Qt::Orientation a_Orientation, int a_Role = Qt::DisplayRole ) const;

	int columnCount( const QModelIndex & a_Parent = QModelIndex() ) const;
	void forgetPath( const QString & a_Path );
	void showTotals( bool a_ShowTotals );

private:
    struct Counts
    {
    	int partial,total,real;
    };

    Counts getCounts( const QString & a_Path ) const;

	// evil? i'll let you decide!
	mutable QHash< QString, Counts > m_CachedCounts;
	int m_BaseColumn;
	bool m_ShowTotals;
};

#endif /* _DRD_BACHDIRMODEL_H_ */
