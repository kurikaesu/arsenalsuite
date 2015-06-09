
#include <qlistwidget.h>
#include <qcombobox.h>
#include <qcolordialog.h>
#include <qpushbutton.h>
#include <qinputdialog.h>

#include "database.h"

#include "resinerror.h"
#include "assettypedialog.h"
#include "pathtemplatesdialog.h"

AssetTypeDialog::AssetTypeDialog( QWidget * parent )
: QDialog( parent )
{
	setupUi( this );
	mAssetTypes = AssetType::select().sorted( "name" );
	resetList();
	connect( mNewType, SIGNAL( clicked() ), SLOT( newType() ) );
	connect( mRemoveType, SIGNAL( clicked() ), SLOT( toggleTypeDisabled() ) );
	connect( mEditType, SIGNAL( clicked() ), SLOT( editType() ) );

	connect( mShowDisabled, SIGNAL( toggled( bool ) ), SLOT( showDisabled( bool ) ) );
	connect( mTypeList, SIGNAL( currentTextChanged( const QString & ) ), SLOT( currentTypeChanged( const QString & ) ) );
	currentTypeChanged("");
}

void AssetTypeDialog::currentTypeChanged( const QString & type )
{
	mCurrentType = AssetType::recordByName( QString(type).replace( "[Disabled]", "" ) );

	bool ir = mCurrentType.isRecord();
	mEditType->setEnabled( ir );
	mRemoveType->setEnabled( ir );
	
	if( mCurrentType.disabled() )
		mRemoveType->setText( "Enable Type" );
	else
		mRemoveType->setText( "Disable Type" );
}

void AssetTypeDialog::resetList()
{
	mTypeList->clear();
	mAssetTypes = mAssetTypes.sorted( "name" );
	foreach( AssetType at, mAssetTypes ) {
		if( at.disabled() && mShowDisabled->isChecked() )
			mTypeList->addItem( at.name() + "[Disabled]" );
		else if( !at.disabled() )
			mTypeList->addItem( at.name() );
	}
}

void AssetTypeDialog::showDisabled( bool sd )
{
	mShowDisabled->setChecked( sd );
	resetList();
}

AssetType AssetTypeDialog::currentType()
{
	return mCurrentType;
}

void AssetTypeDialog::toggleTypeDisabled()
{
	QListWidgetItem * cur = mTypeList->currentItem();
	if( cur && mCurrentType.isRecord() ) {
		if( mCurrentType.disabled() ) {
			cur->setText( mCurrentType.name() );
			mCurrentType.setDisabled( 0 );
			mRemoveType->setText( "Disable Type" );
		} else {
			cur->setText( mCurrentType.name() + "[Disabled]" );
			mCurrentType.setDisabled( 1 );
			mRemoveType->setText( "Enable Type" );
		}
		mCurrentType.commit();
	}
}

void AssetTypeDialog::newType()
{
	EditAssetTypeDialog * eat = new EditAssetTypeDialog( this );
	if( eat->exec() == QDialog::Accepted ) {
		AssetType at = eat->assetType();
		at.commit();
		mAssetTypes += at;
		resetList();
	}
	delete eat;
}

void AssetTypeDialog::accept()
{
	QDialog::accept();
}

void AssetTypeDialog::editType()
{
	EditAssetTypeDialog * eat = new EditAssetTypeDialog( this );
	eat->setAssetType( mCurrentType );
	if( eat->exec() == QDialog::Accepted ) {
		if( mCurrentType.isTask() != eat->assetType().isTask() )
			Element::invalidatePathCache();
		eat->assetType().commit();
	}
	resetList();
	delete eat;
}

EditAssetTypeDialog::EditAssetTypeDialog( QWidget * parent )
: QDialog( parent )
{
	setupUi( this );
	refreshTemplates();
	connect( mEditPathTemplatesButton, SIGNAL( clicked() ), SLOT( editTemplates() ) );
	connect( mColorButton, SIGNAL( clicked() ), SLOT( chooseColor() ) );
}

void EditAssetTypeDialog::setAssetType( const AssetType & at )
{
	mAssetType = at;
	mTypeName->setText( at.name() );
	mNameRE->setText( at.nameRegExp() );
	mTaskCheck->setChecked( at.isTask() );
	mDisabledCheck->setChecked( at.disabled() );
	mPathTemplateCombo->setCurrentIndex( at.pathTemplate().isRecord() ? mTemplates.findIndex( at.pathTemplate() ) : 0 );
	mAllowTimeCheck->setCheckState( at.allowTime() ? Qt::Checked : Qt::Unchecked );
	QPalette p = mColorButton->palette();
	QColor c;
	c.setNamedColor( at.color() );
	p.setColor( QPalette::Button, c );
	mColorButton->setPalette( p );
	mDescriptionEdit->setPlainText( at.description() );
	mTagsEdit->setPlainText( at.tags() );
	mSortNumberSpin->setValue( at.sortNumber() );
	populateSortColumnCombo();
}

AssetType EditAssetTypeDialog::assetType()
{
	mAssetType.setName( mTypeName->text() );
	mAssetType.setNameRegExp( mNameRE->text() );
	mAssetType.setIsTask( mTaskCheck->isChecked() );
	mAssetType.setDisabled( mDisabledCheck->isChecked() ? 1 : 0 );
	mAssetType.setPathTemplate( mTemplates[mPathTemplateCombo->currentIndex()] );
	mAssetType.setAllowTime( mAllowTimeCheck->checkState() == Qt::Checked );
	QPalette p = mColorButton->palette();
	mAssetType.setColor( p.color( QPalette::Button ).name() );
	mAssetType.setDescription( mDescriptionEdit->toPlainText() );
	mAssetType.setTags( mTagsEdit->toPlainText() );
	mAssetType.setSortColumn( mSortColumnCombo->currentText() );
	mAssetType.setSortNumber( mSortNumberSpin->value() );
	return mAssetType;
}

void EditAssetTypeDialog::populateSortColumnCombo()
{
	Table * t = Database::current()->tableByName( mAssetType.elementType().name() );
	QStringList sortColumns;
	sortColumns += "Display Name";
	if( t ) {
		FieldList fl = t->schema()->fields();
		foreach( Field * f, fl ) {
			sortColumns += f->methodName();
		}
	}
	mSortColumnCombo->clear();
	mSortColumnCombo->addItems( sortColumns );
	int idx = mSortColumnCombo->findText( mAssetType.sortColumn(), Qt::MatchFixedString );
	if( idx >= 0 )
		mSortColumnCombo->setCurrentIndex( idx );
}

void EditAssetTypeDialog::accept()
{
	if( mTypeName->text().isEmpty() ) {
		ResinError::nameEmpty( this, "Asset Type" );
		return;
	}
	
	AssetType cur = AssetType::recordByName( mTypeName->text() );
	if( cur.isRecord() && cur.key() != mAssetType.key() ) {
		ResinError::nameTaken( this, mTypeName->text() );
		return;
	}
	
	assetType();
	QDialog::accept();
}

void EditAssetTypeDialog::refreshTemplates()
{
	mTemplates = PathTemplate::select().sorted( "name" );
	PathTemplate none;
	none.setName( "None" );
	mTemplates.insert( mTemplates.begin(), none );
	mPathTemplateCombo->clear();
	mPathTemplateCombo->addItems( mTemplates.names() );
}

void EditAssetTypeDialog::editTemplates()
{
	PathTemplatesDialog ptd( this );
	ptd.exec();
}

void EditAssetTypeDialog::chooseColor()
{
	QPalette p = mColorButton->palette();
	QColor c = QColorDialog::getColor( p.color( QPalette::Button ), this );
	p.setColor( QPalette::Button, c );
	mColorButton->setPalette( p );
}

