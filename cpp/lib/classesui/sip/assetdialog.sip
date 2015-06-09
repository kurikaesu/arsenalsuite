
class AssetDialog : QDialog
{
%TypeHeaderCode
#include <assetdialog.h>
%End
public:
	AssetDialog( const Element & parEl, QWidget * parent /TransferThis/ = 0 );

	void setAssetType( const AssetType & at );

	ElementList created();

	virtual void accept();
	virtual void reject();

	void setAssetTemplatesEnabled( bool );
	
	void setCreateAssetTemplates( bool );

	void setPathTemplatesEnabled( bool );

	void setAsset( const Element & );

public slots:
	void assetTypeChanged( const QString & );

	void assetTemplateChanged( const AssetTemplate & at );

	void create();

	void updateStatusLabel( const QString & );

};


