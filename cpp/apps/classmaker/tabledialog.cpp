
#include <qmenu.h>
#include <qlineedit.h>
#include <qcheckbox.h>
#include <qcombobox.h>

#include "schema.h"
#include "tableschema.h"

#include "tabledialog.h"
#include "ui_docsdialogui.h"

TableDialog::TableDialog( TableSchema * table, QWidget * parent )
: QDialog( parent )
, mTable( table )
{
	mUI.setupUi( this );

	connect( mUI.mProjectPreload, SIGNAL( toggled( bool ) ), SLOT( useProjectPreload( bool ) ) );
	connect( mUI.mName, SIGNAL( textChanged( const QString & ) ), SLOT( slotNameChanged( const QString & ) ) );
	
	bool hasParent = bool(table->parent());
	mUI.mName->setText( table->tableName() );
	mUI.mClass->setText( table->className() );
	mUI.mPreload->setChecked( table->isPreloadEnabled() );
	mUI.mUseCodeGen->setChecked( table->useCodeGen() );
	mUI.mUseCodeGen->setEnabled( !hasParent );
	mUI.mCreatePKeyCheck->setChecked( !hasParent );
	mUI.mCreatePKeyCheck->setEnabled( !hasParent );
	mUI.mExpireKeyCacheCheck->setChecked( table->expireKeyCache() );
	
	FieldList fl = table->columns();
	foreach( Field * f, fl )
		mUI.mFieldCombo->addItem( f->name() );
	if( !table->projectPreloadColumn().isEmpty() ) {
		mUI.mProjectPreload->setChecked( true );
		mUI.mFieldCombo->setCurrentIndex( mUI.mFieldCombo->findText( table->projectPreloadColumn() ) );
	}
	connect( mUI.mEditDocsButton, SIGNAL( clicked() ), SLOT( editDocs() ) );
}

void TableDialog::applySettings()
{
	mTable->setTableName( mUI.mName->text() );
	mTable->setClassName( mUI.mClass->text() );
	mTable->setPreloadEnabled( mUI.mPreload->isChecked() );
	mTable->setProjectPreloadColumn( mUI.mProjectPreload->isChecked() ? mUI.mFieldCombo->currentText() : QString::null );
	mTable->setUseCodeGen( mUI.mUseCodeGen->isChecked() );
	mTable->setExpireKeyCache( mUI.mExpireKeyCacheCheck->isChecked() );
	if( mUI.mCreatePKeyCheck->isChecked() )
		new Field( mTable, "key" + mTable->tableName(), Field::UInt, Field::PrimaryKey );
}

TableSchema * TableDialog::createTable( QWidget * parent, Schema * schema, TableSchema * parTable )
{
	TableSchema * table = new TableSchema( schema );
	if( parTable ) table->setParent( parTable );
	TableDialog * td = new TableDialog( table, parent );
	td->setWindowTitle( "Create Table" );
	if( td->exec() == QDialog::Accepted ) {
		td->applySettings();
	} else {
		delete table;
		table = 0;
	}
	delete td;
	return table;
}

bool TableDialog::modifyTable( QWidget * parent, TableSchema * table )
{
	bool ret = false;
	TableDialog * td = new TableDialog( table, parent );
	td->setWindowTitle( "Modify Table" );
	td->mUI.mCreatePKeyCheck->hide();
	td->mUI.mCreatePKeyCheck->setChecked( false );
	if( td->exec() == QDialog::Accepted ) {
		td->applySettings();
		ret = true;
	}
	delete td;
	return ret;
}

void TableDialog::useProjectPreload( bool pp )
{
	mUI.mFieldCombo->setEnabled( pp );
}

void TableDialog::slotNameChanged( const QString & name )
{
	mUI.mClass->setText( name );
}

void TableDialog::editDocs()
{
	QDialog * dialog = new QDialog(this);
	Ui::DocsDialogUI ui;
	ui.setupUi(dialog);
	ui.mTextEdit->setPlainText( mTable->docs() );
	if( dialog->exec() == QDialog::Accepted )
		mTable->setDocs( ui.mTextEdit->toPlainText() );
	delete dialog;
}
