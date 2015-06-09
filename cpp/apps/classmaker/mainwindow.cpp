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
 
#include <qaction.h>
#include <qfiledialog.h>
#include <qfileinfo.h>
#include <qtoolbar.h>
#include <qtreewidget.h>
#include <qmenu.h>
#include <qmessagebox.h>
#include <QCloseEvent>

#include "blurqt.h"
#include "database.h"
#include "tableschema.h"
#include "indexschema.h"
#include "schema.h"
#include "path.h"
#include "sourcegen.h"

#include "iconserver.h"

#include "mainwindow.h"
#include "createdatabasedialog.h"
#include "tabledialog.h"
#include "fielddialog.h"
#include "indexdialog.h"

#include "supermodel.h"

struct TableItem : public ItemBase
{
	TableSchema * mTable;
	TableItem( TableSchema * table = 0 ) : mTable(table) {}
	bool operator==(const TableItem & other) const { return mTable == other.mTable; }
	QVariant modelData( const QModelIndex & idx, int role ) const
	{
		if( role == Qt::DisplayRole && mTable ) {
			switch( idx.column() ) {
				case 0: return mTable->className();
				case 1: return "Table";
				case 2: return mTable->tableName();
			}
		}
		return QVariant();
	}
};

typedef TemplateDataTranslator<TableItem> TableTranslator;

struct FieldItem : public ItemBase
{
	Field * mField;
	FieldItem( Field * field = 0 ) : mField(field) {}
	QVariant modelData( const QModelIndex & idx, int role ) const
	{
		if( role == Qt::DisplayRole && mField ) {
			switch( idx.column() ) {
				case 0: return mField->methodName();
				case 1: return QString(mField->flag( Field::LocalVariable ) ? "Variable" : "Field") + " [" + mField->typeString() + "]";
				case 2: return mField->name();
				case 3: return mField->displayName();
			}
		}
		return QVariant();
	}
};
typedef TemplateDataTranslator<FieldItem> FieldTranslator;

struct IndexItem : public ItemBase
{
	IndexSchema * mIndex;
	IndexItem( IndexSchema * schema = 0 ) : mIndex(schema) {}
	QVariant modelData( const QModelIndex & idx, int role ) const
	{
		if( role == Qt::DisplayRole && mIndex ) {
			switch( idx.column() ) {
				case 0: return mIndex->name();
				case 1: return "Index";
			}
		}
		return QVariant();
	}
};
typedef TemplateDataTranslator<IndexItem> IndexTranslator;

/* This class defines/builds the relationships between the different types in the model */
class SchemaTreeBuilder : public ModelTreeBuilder
{
public:
	SchemaTreeBuilder( SuperModel * parent ) : ModelTreeBuilder( parent )
	{
		mTableTranslator = new TableTranslator(this);
		mFieldTranslator = new FieldTranslator(this);
		mIndexTranslator = new IndexTranslator(this);
		mStandardTranslator = new StandardTranslator(this);
	}

	virtual bool hasChildren( const QModelIndex & idx, SuperModel * )
	{
		// All tables at least have Fields and Indexes groups
		return TableTranslator::isType(idx);
	}
	virtual void loadChildren( const QModelIndex & parentIndex, SuperModel * model )
	{
		if( TableTranslator::isType(parentIndex) ) {
			TableSchema * table = TableTranslator::data(parentIndex).mTable;
			// Add child tables
			mTableTranslator->appendList( table->children(), parentIndex );
			
			// Add fields inside a fields group
			QModelIndex fieldGroup = mStandardTranslator->append( parentIndex );
			model->setData( fieldGroup, "Fields", Qt::DisplayRole );
			mFieldTranslator->appendList( table->ownedFields(), fieldGroup );

			// Add Indexes inside an index group
			QModelIndex indexGroup = mStandardTranslator->append( parentIndex );
			model->setData( indexGroup, "Indexes", Qt::DisplayRole );
			mIndexTranslator->appendList( table->indexes(), indexGroup );
		}
	}
	int typeScore( const QModelIndex & idx ) {
		if( TableTranslator::isType(idx) ) return 1;
		if( StandardTranslator::isType(idx) )
			return idx.data(Qt::DisplayRole).toString() == "Fields" ? 2 : 3;
		return 0;
	}
	int compare( const QModelIndex & a, const QModelIndex & b, QList<int> , bool )
	{
		return typeScore(a) - typeScore(b);
	}

	TableTranslator * mTableTranslator;
	FieldTranslator * mFieldTranslator;
	IndexTranslator * mIndexTranslator;
	StandardTranslator * mStandardTranslator;
};

MainWindow::MainWindow( Schema * schema )
: QMainWindow( 0 )
, mSchema( 0 )
, mChanges( false )
{
	mUI.setupUi( this );

	QAction * openSchema = new QAction( QIcon(::icon( "fileopen.png" )), "Open Schema", this );
	QAction * importSchema = new QAction( "Import Schema", this );
	QAction * saveSchema = new QAction( QIcon(::icon( "filesave.png" ) ), "Save Schema", this );
	QAction * saveSchemaAs = new QAction( QIcon(::icon( "filesaveas.png" ) ), "Save Schema As", this );
	QAction * outputSource = new QAction( QIcon(::icon( "exec.png" ) ), "Output Source", this );
	QAction * createDatabase = new QAction( QIcon(::icon( "create_database.png" ) ), "Create Database", this );
	QAction * diff = new QAction( QIcon(::icon( "diff.png" )), "Generate Diff with Schema...", this );
	
	QToolBar * toolbar = addToolBar( "Main Toolbar" );
	toolbar->addAction( openSchema );
	toolbar->addAction( importSchema );
	toolbar->addAction( saveSchema );
	toolbar->addAction( saveSchemaAs );
	toolbar->addAction( outputSource );
	toolbar->addAction( createDatabase );
	toolbar->addAction( diff );
	
	connect( openSchema, SIGNAL( triggered() ), SLOT( slotOpenSchema() ) );
	connect( importSchema, SIGNAL( triggered() ), SLOT( slotImportSchema() ) );
	connect( saveSchema, SIGNAL( triggered() ), SLOT( slotSaveSchema() ) );
	connect( saveSchemaAs, SIGNAL( triggered() ), SLOT( slotSaveSchemaAs() ) );
	connect( outputSource, SIGNAL( triggered() ), SLOT( slotOutputSource() ) );
	connect( createDatabase, SIGNAL( triggered() ), SLOT( slotCreateDatabase() ) );
	connect( diff, SIGNAL( triggered() ), SLOT( slotGenerateDiff() ) );
	
	connect( mUI.mTreeView, SIGNAL( customContextMenuRequested( const QPoint & ) ), SLOT( showContextMenu( const QPoint & ) ) );
	mUI.mTreeView->setContextMenuPolicy( Qt::CustomContextMenu );

	mUI.mTreeView->setRootIsDecorated( true );
	mUI.mTreeView->setSortingEnabled( true );

	mModel = new SuperModel(mUI.mTreeView);
	QStringList hl;
	hl << "Name" << "Type" << "Db Name" << "Display Name";
	mModel->setHeaderLabels( hl );

	mTreeBuilder = new SchemaTreeBuilder(mModel);
	mModel->setTreeBuilder( mTreeBuilder );

	mUI.mTreeView->setModel(mModel);
	
	setSchema( 0 );
}

void MainWindow::expandChildTables( const QModelIndex & idx )
{
	for( ModelIter it = idx.isValid() ? ModelIter(idx.child(0,0)) : ModelIter(mModel); it.isValid(); ++it )
	{
		if( TableTranslator::isType(*it) ) {
			TableSchema * ts = TableTranslator::data(*it).mTable;
			if( ts->children().size() ) {
				mUI.mTreeView->expand( *it );
				expandChildTables( *it );
			}
		}
	}
}

static const char * sFileDialogFilterString = "Schemas (*.xml *.schema);; All Files (*)";

void MainWindow::slotOpenSchema()
{
	openSchema( QFileDialog::getOpenFileName( this, "Choose A Schema", QString::null, sFileDialogFilterString ) );
}

void MainWindow::slotImportSchema()
{
	QString schemaFile = QFileDialog::getOpenFileName( this, "Choose A Schema", QString::null, sFileDialogFilterString );
	if( schemaFile.size() ) {
		if( mSchema ) {
			mSchema->mergeXmlSchema( schemaFile, /*isfile=*/true, /*ignoreDocs*/false );
		} else
			openSchema( schemaFile );
		setSchema( mSchema );
	}
}

void MainWindow::openSchema( const QString & schema )
{
	if( QFile::exists( schema ) ) {
		mFileName = schema;
		setSchema( Schema::createFromXmlSchema( schema, /*isfile=*/true, /*ignoreDocs*/false ) );
	}
}

void MainWindow::setSchema( Schema * schema )
{
	if( mSchema && mSchema != schema ) delete mSchema;
	if( !schema ) schema = new Schema();

	mSchema = schema;
	mModel->clear();

	// Add the top-level tables, they will add their children
	foreach( TableSchema * t, mSchema->tables() )
		if( !t->parent() )
			addTable( t );
	expandChildTables( QModelIndex() );
	mChanges = false;
}

void MainWindow::setFileName( const QString & fn )
{
	mFileName = fn;
}

void MainWindow::slotSaveSchema()
{
	if( QFile::exists( mFileName ) )
		mSchema->writeXmlSchema( mFileName );
	else
		slotSaveSchemaAs();
	mChanges = false;
}

void MainWindow::slotSaveSchemaAs()
{
	exportSchema( 
		QFileDialog::getSaveFileName( 
			this, "Choose location to save Schema", mFileName, "Schemas (*.xml)" ) );
	mChanges = false;
}

void MainWindow::slotCreateDatabase( TableSchema * ts )
{
	CreateDatabaseDialog * cdd = new CreateDatabaseDialog( mSchema, this );
	if( ts )
		cdd->setTableSchema( ts );
	cdd->exec();
	delete cdd;
}

void MainWindow::exportSchema( const QString & sf )
{
	if( !sf.isEmpty() && mSchema ) {
		mSchema->writeXmlSchema( sf );
		mFileName = sf;
	}
}

void MainWindow::slotOutputSource()
{
	QString dir = QFileDialog::getExistingDirectory(
		this, "Select folder to save source", QFileInfo( mFileName ).path() );
	if( Path::mkdir( dir + "/autoimp" ) && Path::mkdir( dir + "/autocore" ) )
		writeSource( mSchema, dir );
}

void MainWindow::slotGenerateDiff()
{
	if( !mSchema ) {
		// TODO warn user
		return;
	}
	QString otherSchemaPath = QFileDialog::getOpenFileName( this, "Choose A Schema", QString::null, "Schemas (*.xml)" );
	if( otherSchemaPath.isEmpty() ) return;
	
	Schema * otherSchema = Schema::createFromXmlSchema( otherSchemaPath, true, false );
	if( otherSchema ) {
		LOG_1( Schema::diff( mSchema, otherSchema ) );
	}
}

void MainWindow::closeEvent( QCloseEvent * ce )
{
	if( mChanges ) {
		int result = QMessageBox::warning( this, "Unsaved Changes", "Do you wish to save your changes?",
			QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel );
		
		if( result == QMessageBox::Cancel ) {
			ce->ignore();
			return;
		}
		
		if( result == QMessageBox::Yes ) {
			if( !mFileName.isEmpty() )
				exportSchema( mFileName );
			else
				slotSaveSchemaAs();
		}
	}
	
	ce->accept();
}

void MainWindow::showContextMenu( const QPoint & )
{
	QModelIndexList selectedRows = mUI.mTreeView->selectedRows();
	printf( "%i rows selected\n", selectedRows.size() );
	if( selectedRows.size() > 1 ) return;

	QModelIndex idx;
	if( selectedRows.size() == 1 ) idx = selectedRows[0];

	QMenu * m = new QMenu( this );
	QAction * newTable = 0, * newTableInherit = 0, * newField = 0, * newIndex = 0, * modifyTable = 0, * removeTable = 0, * createVerifyTable = 0;
	QAction * modifyField = 0, * removeField = 0, * modifyIndex = 0, * removeIndex = 0;
	TableSchema * table = 0;
	if( !idx.isValid() || TableTranslator::isType(idx) ) {
		newTable = m->addAction( "Create Table" );
		if( TableTranslator::isType(idx) ) {
			newTableInherit = m->addAction( "Create Inherited Table" );
			table = TableTranslator::data(idx).mTable;
		}
	}

	if( StandardTranslator::isType(idx) ) {
		QString name = idx.data(Qt::DisplayRole).toString();
		table = TableTranslator::data(idx.parent()).mTable;
		if( name == "Fields" )
			newField = m->addAction( "Create Field/Variable" );
		if( name == "Indexes" )
			newIndex = m->addAction( "Create Index" );
	}

	if( TableTranslator::isType(idx) ) {
		m->addSeparator();
		modifyTable = m->addAction( "Modify Table" );
		removeTable = m->addAction( "Remove Table" );
		m->addSeparator();
		createVerifyTable = m->addAction( "Create or Verify Table in Database..." );
	}

	if( FieldTranslator::isType(idx) ) {
		modifyField = m->addAction( "Modify Field" );
		removeField = m->addAction( "Remove Field" );
	} else if ( IndexTranslator::isType(idx) ) {
		modifyIndex = m->addAction( "Modify Index" );
		removeIndex = m->addAction( "Remove Index" );
	}
	
	QAction * res = m->exec( QCursor::pos() );
	if( !res ) {
		delete m;
		return;
	}

	if( res == newTable ) {
		addTable( TableDialog::createTable( this, mSchema,  0 ) );
	} else if( res == newTableInherit ) {
		addTable( TableDialog::createTable( this, mSchema, table ) );
	} else if( res == modifyTable ) {
		if( TableDialog::modifyTable( this, table ) ) {
			mModel->dataChange(idx);
			mChanges = true;
		}
	} else if( res == removeTable ) {
		mModel->remove(idx);
		delete table;
		mChanges = true;
	} else if( res == createVerifyTable ) {
		slotCreateDatabase( table );
	} else if( res == newField ) {
		addField( FieldDialog::createField( this, table ) );
	} else if( res == modifyField ) {
		if( FieldDialog::modifyField( this, FieldTranslator::data(idx).mField ) ) {
			mModel->dataChange(idx);
			mChanges = true;
		}
	} else if( res == removeField ) {
		Field * f = FieldTranslator::data(idx).mField;
		mModel->remove(idx);
		delete f;
		mChanges = true;
	} else if( res == newIndex ) {
		addIndex( IndexDialog::createIndex( this, table ) );
	} else if( res == modifyIndex ) {
		if( IndexDialog::modifyIndex( this, IndexTranslator::data(idx).mIndex ) ) {
			mModel->dataChange(idx);
			mChanges = true;
		}
	} else if( res == removeIndex ) {
		IndexSchema * i = IndexTranslator::data(idx).mIndex;
		mModel->remove(idx);
		delete i;
		mChanges = true;
	}
	delete m;
}

void MainWindow::addTable( TableSchema * table )
{
	if( !table )
		return;

	QModelIndex parent;
	if( table->parent() )
		parent = TableTranslator::findFirst( ModelIter(mModel,ModelIter::Recursive), table->parent() );
	mTreeBuilder->mTableTranslator->append( table, parent );
	mChanges = true;
}

void MainWindow::addField( Field * field )
{
	if( !field ) return;
	// Find the table
	QModelIndex parent = TableTranslator::findFirst( ModelIter(mModel,ModelIter::Recursive), field->table() );
	if( parent.isValid() ) {
		// Find the Fields group
		parent = ModelIter(parent.child(0,0)).findFirst( 0, "Fields" );
		if( parent.isValid() )
			mTreeBuilder->mFieldTranslator->append( field, parent );
	}
	mChanges = true;
}

void MainWindow::addIndex( IndexSchema * index )
{
	if( !index ) return;
	// Find the table
	QModelIndex parent = TableTranslator::findFirst( ModelIter(mModel,ModelIter::Recursive), index->table() );
	if( parent.isValid() ) {
		// Find the Indexes group
		parent = ModelIter(parent.child(0,0)).findFirst( 0, "Indexes" );
		if( parent.isValid() )
			mTreeBuilder->mIndexTranslator->append( index, parent );
	}
	mChanges = true;
}

