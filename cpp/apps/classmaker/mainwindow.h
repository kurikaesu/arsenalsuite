
/*
 *
 * Copyright 2003, 2004 Blur Studio Inc.
 *
 * This file is part of the Resin software package.
 *
 * Assburner is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Assburner is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Resin; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
 
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <qmap.h>
#include <qmainwindow.h>

#include "ui_mainwindowui.h"

class QPoint;

namespace Stone {
class Database;
class Field;
class IndexSchema;
class Schema;
class TableSchema;
}
using namespace Stone;

class SuperModel;
class SchemaTreeBuilder;

class MainWindow : public QMainWindow
{
Q_OBJECT
public:
	MainWindow( Schema * schema = 0 );
	
public slots:
	void slotOpenSchema();
	void slotImportSchema();
	void slotSaveSchema();
	void slotSaveSchemaAs();
	void slotOutputSource();
	/// Creates/verifys the table only if supplied
	void slotCreateDatabase(TableSchema * ts = 0);

	void slotGenerateDiff();
	
	void openSchema( const QString & );
	void exportSchema( const QString & );
	void setFileName( const QString & );

	void showContextMenu( const QPoint & pos );

	void setSchema( Schema * schema );
	
	void addTable( TableSchema * table );
	void addIndex( IndexSchema * index );
	void addField( Field * field );
	
protected:
	
	virtual void closeEvent( QCloseEvent * );
	void expandChildTables( const QModelIndex & );
	
	Schema * mSchema;
	QString mFileName;
	SuperModel * mModel;
	SchemaTreeBuilder * mTreeBuilder;
	
	bool mChanges;
	
	Ui::MainWindowUI mUI;
};

#endif // MAIN_WINDOW_H

