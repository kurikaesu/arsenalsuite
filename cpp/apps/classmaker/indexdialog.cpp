
#include <qlineedit.h>
#include <qlistwidget.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qcheckbox.h>

#include "tableschema.h"
#include "indexschema.h"

#include "indexdialog.h"

IndexDialog::IndexDialog( IndexSchema * index, QWidget * parent )
: QDialog( parent )
, mIndex( index )
{
	mUI.setupUi( this );

	TableSchema * table = mIndex->table();
	mUI.mName->setText( mIndex->name() );
	QStringList cols;
	FieldList fl = mIndex->columns();
	for( FieldIter it = fl.begin(); it != fl.end(); ++it )
	{
		QString cn = (*it)->name();
		cols += cn;
		mUI.mFieldList->addItem( cn );
	}
	fl = table->columns();
	for( FieldIter it = fl.begin(); it != fl.end(); ++it )
	{
		if( !cols.contains( (*it)->name() ) )
			mUI.mFieldCombo->addItem( (*it)->name() );
	}
	mUI.mAddField->setEnabled( mUI.mFieldCombo->count() > 0 );
	mUI.mMultiCheck->setChecked( index->holdsList() );
	mUI.mCacheCheck->setChecked( index->useCache() );
	connect( mUI.mAddField, SIGNAL( clicked() ), SLOT( addField() ) );
}

void IndexDialog::applySettings()
{
	mIndex->setName( mUI.mName->text() );
	mIndex->setHoldsList( mUI.mMultiCheck->isChecked() );
	mIndex->setUseCache( mUI.mCacheCheck->isChecked() );
}

void IndexDialog::addField()
{
	Field * f = mIndex->table()->field( mUI.mFieldCombo->currentText() );
	mUI.mFieldCombo->removeItem( mUI.mFieldCombo->currentIndex() );
	mIndex->addField( f );
	mUI.mFieldList->addItem( f->name() );
}

IndexSchema * IndexDialog::createIndex( QWidget * parent, TableSchema * table )
{
	IndexSchema * index = new IndexSchema( "temp", table, true );
	IndexDialog * td = new IndexDialog( index, parent );
	td->setWindowTitle( "Create Index" );
	if( td->exec() == QDialog::Accepted ) {
		td->applySettings();
	} else {
		delete index;
		index = 0;
	}
	delete td;
	return index;
}

bool IndexDialog::modifyIndex( QWidget * parent, IndexSchema * index )
{
	bool ret = false;
	IndexDialog * td = new IndexDialog( index, parent );
	td->setWindowTitle( "Modify Index" );
	if( td->exec() == QDialog::Accepted ) {
		td->applySettings();
		ret = true;
	}
	delete td;
	return ret;
}

