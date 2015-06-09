

#ifndef ASSET_TYPE_DIALOG_UI
#define ASSET_TYPE_DIALOG_UI

#include <qdialog.h>

#include "classesui.h"

#include "ui_assettypedialogui.h"
#include "ui_editassettypedialogui.h"

#include "assettype.h"
#include "pathtemplate.h"

class CLASSESUI_EXPORT AssetTypeDialog : public QDialog, public Ui::AssetTypeDialogUI
{
Q_OBJECT
public:
	AssetTypeDialog( QWidget * parent = 0 );

	void accept();

	AssetType currentType();

public slots:
	void resetList();

	void showDisabled( bool );

	void newType();
	void toggleTypeDisabled();
	void editType();
	void currentTypeChanged( const QString & text );

protected:

	AssetTypeList mAssetTypes;
	AssetType mCurrentType;
};

class CLASSESUI_EXPORT EditAssetTypeDialog : public QDialog, public Ui::EditAssetTypeDialogUI
{
Q_OBJECT
public:
	EditAssetTypeDialog( QWidget * parent = 0 );

	void setAssetType( const AssetType & );
	AssetType assetType();

	void accept();

public slots:
	void editTemplates();

	void chooseColor();

protected:
	void refreshTemplates();
	void populateSortColumnCombo();

	AssetType mAssetType;
	PathTemplateList mTemplates;
};

#endif // ASSET_TYPE_DIALOG_UI

