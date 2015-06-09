
#include <qmenu.h>
#include <qlineedit.h>
#include <qgroupbox.h>
#include <qcombobox.h>
#include <qcheckbox.h>

#include "fielddialog.h"

#include "blurqt.h"
#include "database.h"
#include "tableschema.h"
#include "schema.h"
#include "field.h"
#include "ui_docsdialogui.h"

FieldDialog::FieldDialog( Field * field, QWidget * parent )
: QDialog( parent )
, mField( field )
{
	mUI.setupUi( this );

	TableSchemaList tables = field->table()->schema()->tables();
	foreach( TableSchema * t, tables )
		mUI.mTableCombo->addItem( t->className() );
	
	mUI.mName->setText( field->name() );
	mUI.mType->setCurrentIndex( mUI.mType->findText( field->variantTypeString() ) );
	mUI.mLocal->setChecked( field->flag( Field::LocalVariable ) );
	mUI.mDefaultSelectCheck->setChecked( !field->flag( Field::NoDefaultSelect ) );
	mUI.mMethod->setText( field->methodName() );
	mUI.mPluralMethod->setText( field->pluralMethodName() );
	mUI.mIndexName->setText( field->methodName() );
	mUI.mDisplayName->setText( field->displayName() );
	
	bool fk = field->flag( Field::ForeignKey );
	mUI.mFKGroup->setEnabled( fk );
	mUI.mListCheck->setChecked( field->flag( Field::Unique ) );
	mUI.mDisplayNameCheck->setChecked( field->flag( Field::TableDisplayName ) );
	
	if( fk ) {
		mUI.mTableCombo->setCurrentIndex( mUI.mTableCombo->findText( field->foreignKey() ) );
		mUI.mType->setCurrentIndex( mUI.mType->findText( "Foreign Key" ) );
	} else if( field->flag( Field::PrimaryKey ) )
		mUI.mType->setCurrentIndex( mUI.mType->findText( "Primary Key" ) );

	bool hi = field->hasIndex();
	mUI.mIndexGroup->setChecked( hi );
	if( hi ) {
		QString in = field->index()->name();
		if( !in.isEmpty() )
			mUI.mIndexName->setText( in );
		mUI.mUseCacheCheck->setChecked( field->index()->useCache() );
	}
	
	mUI.mReverseAccess->setChecked( field->flag( Field::ReverseAccess ) );
	
	mUI.mDeleteModeCombo->setCurrentIndex( 0 );
	if( fk && hi ) {
		mUI.mDeleteModeCombo->setEnabled( true );
		mUI.mDeleteModeCombo->setCurrentIndex( field->indexDeleteMode() );
    }
	
	connect( mUI.mType, SIGNAL( activated( const QString & ) ),
		SLOT( typeChanged( const QString & ) ) );
	connect( mUI.mName, SIGNAL( textChanged( const QString & ) ),
		SLOT( nameChanged( const QString & ) ) );
	connect( mUI.mMethod, SIGNAL( textChanged( const QString & ) ),
		SLOT( methodNameChanged( const QString & ) ) );
	connect( mUI.mEditDocsButton, SIGNAL( clicked() ), SLOT( editDocs() ) );
}

void FieldDialog::applySettings()
{
	mField->setName( mUI.mName->text() );
	mField->setMethodName( mUI.mMethod->text() );
	mField->setDisplayName( mUI.mDisplayName->text() );
	mField->setPluralMethodName( mUI.mPluralMethod->text() );

	QString ts = mUI.mType->currentText();
	mField->setFlag( Field::Unique, mUI.mListCheck->isChecked() );
	mField->setFlag( Field::TableDisplayName, mUI.mDisplayNameCheck->isChecked() );
	mField->setFlag( Field::NoDefaultSelect, !mUI.mDefaultSelectCheck->isChecked() );
	if( ts == "Foreign Key" ) {
		mField->setFlag( Field::ForeignKey, true );
		mField->setForeignKey( mUI.mTableCombo->currentText() );
		mField->setType( Field::UInt );
		mField->setFlag( Field::PrimaryKey, false );
	} else if( ts == "Primary Key" ) {
		mField->setType( Field::UInt );
		mField->setFlag( Field::PrimaryKey, true );
	} else {
		mField->setForeignKey( QString::null );
		mField->setType( Field::stringToType( ts ) );
		mField->setFlag( Field::PrimaryKey, false );
		mField->setFlag( Field::LocalVariable, mUI.mLocal->isChecked() );
	}
	
	bool hi = mUI.mIndexGroup->isChecked();
	if( hi ) {
		mField->setHasIndex( true, mField->flag( Field::ForeignKey ) ? mUI.mDeleteModeCombo->currentIndex() : Field::DoNothingOnDelete );
		mField->index()->setName( mUI.mIndexName->text() );
		mField->index()->setUseCache( mUI.mUseCacheCheck->isChecked() );
	} else
		mField->setHasIndex( false );
		
	mField->setFlag( Field::ReverseAccess, mUI.mReverseAccess->isChecked() );
}

Field * FieldDialog::createField( QWidget * parent, TableSchema * table )
{
	Field * field = new Field( table, "", Field::UInt );
	
	if( !field->table() ) {
		LOG_1( "Couldn't create field" );
		delete field;
		return 0;
	}
		
	FieldDialog * td = new FieldDialog( field, parent );
	td->setWindowTitle( "Create Field" );
	if( td->exec() == QDialog::Accepted ) {
		td->applySettings();
	} else {
		delete field;
		field = 0;
	}
	delete td;
	return field;
}

bool FieldDialog::modifyField( QWidget * parent, Field * field )
{
	bool ret = false;
	FieldDialog * td = new FieldDialog( field, parent );
	td->setWindowTitle( "Modify Field" );
	if( td->exec() == QDialog::Accepted ) {
		td->applySettings();
		ret = true;
	}
	delete td;
	return ret;
}

void FieldDialog::typeChanged( const QString & type )
{
	bool et = ( type == "Foreign Key" );
	mUI.mFKGroup->setEnabled( et );
	nameChanged( mUI.mName->text() );
}

void FieldDialog::methodNameChanged( const QString & name )
{
	mUI.mPluralMethod->setText( pluralizeName(mLastMethodName) == mUI.mPluralMethod->text() ? pluralizeName( mUI.mMethod->text() ) : mField->pluralMethodName() );
	if( name.toLower() == "name" )
		mUI.mDisplayNameCheck->setChecked(true);
	mLastMethodName = name;
}

void FieldDialog::nameChanged( const QString & name )
{
	bool dispIsChanged = mField->displayName() != mField->generatedDisplayName();
	if( name.left( 3 ) == "key" ) {
		mUI.mType->setCurrentIndex( mUI.mType->findText( "Primary Key" ) );
		mUI.mMethod->setText( "key" );
	} else if( name.left( 4 ) == "fkey" ) {
		QString tn = name.mid( 4 );
		TableSchema * t = mField->table()->schema()->tableByClass( tn );
		if( !t )
			t = mField->table()->schema()->tableByName( tn );
		if( t ) {
			mUI.mTableCombo->setCurrentIndex( mUI.mTableCombo->findText( t->className() ) );
			mUI.mMethod->setText( name.mid( 4 ) );
			mUI.mType->setCurrentIndex( mUI.mType->findText( "Foreign Key" ) );
			mUI.mFKGroup->setEnabled( true );
		}
	} else
		mUI.mMethod->setText( name );
	if( !dispIsChanged ) {
		mField->setDisplayName( mField->generatedDisplayName() );
		mUI.mDisplayName->setText( mField->displayName() );
	}
}

void FieldDialog::editDocs()
{
	QDialog * dialog = new QDialog(this);
	Ui::DocsDialogUI ui;
	ui.setupUi(dialog);
	ui.mTextEdit->setPlainText( mField->docs() );
	if( dialog->exec() == QDialog::Accepted )
		mField->setDocs( ui.mTextEdit->toPlainText() );
	delete dialog;
}
