//-------------------------------------------------------------------------------------------------
/*
 * BachFolderBrowserTreeView.h
 *
 *  Created on: Jun 24, 2009
 *      Author: david.morris
 */

#ifndef BACHFOLDERBROWSERTREEVIEW_H_
#define BACHFOLDERBROWSERTREEVIEW_H_

#include <QtGui>
#include "recordtreeview.h"
#include "recordsupermodel.h"

class BachDirModel;

//-------------------------------------------------------------------------------------------------
/*
 *
 */
class BachFolderBrowserTreeView : public QTreeView
{
Q_OBJECT;
//-------------------------------------------------------------------------------------------------
public:
	struct ColumnStructEx 
	{
		const char * name;
		const char * iniName;
		const char * toolTip;
		int defaultSize;
		int defaultPos;
		bool defaultHidden;
	};

	enum Columns
	{
		Column_Path 			= 0,
		Column_Size 			= 1,
		Column_Type 			= 2,
		Column_Date 			= 3,
		Column_OnDisk 			= 4,
		Column_Imported			= 5,
		Column_ImportedTotal 	= 6,
		Column_Last				= 7
	};

//-------------------------------------------------------------------------------------------------
	BachFolderBrowserTreeView( QWidget * a_Parent );

	const BachDirModel * getDirModel() const { return m_Model; }
	BachDirModel * getDirModel() { return m_Model; }

	QString getSelectedPath() const;
	void forgetPath( const QString & a_Path );
	void showTotals( bool a_ShowTotals );

	void loadState();
	void saveState();

	static QString getTitle( Columns a_Which );
	static QString getToolTip( Columns a_Which );

public slots:
	void onCollapsed( const QModelIndex & index );
	void onExpanded( const QModelIndex & index );

	void setupColumns( IniConfig & ini, const ColumnStructEx columns [] );
	void saveColumns( IniConfig & ini, const ColumnStructEx columns [] );
	void setupTreeView( IniConfig & ini, const ColumnStructEx columns [] );
	void saveTreeView( IniConfig & ini, const ColumnStructEx columns [] );

//-------------------------------------------------------------------------------------------------
private:
	BachDirModel * m_Model;
};

#endif /* BACHFOLDERBROWSERTREEVIEW_H_ */
