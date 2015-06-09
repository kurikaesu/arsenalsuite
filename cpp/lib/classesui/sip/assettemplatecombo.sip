
class AssetTemplateCombo : QComboBox
{
%TypeHeaderCode
#include <assettemplatecombo.h>
%End
public:
	AssetTemplateCombo( QWidget * parent /TransferThis/ );

	void setAssetType( const AssetType & );
	void setProject( const Project & );

	AssetType assetType();
	Project project();

	AssetTemplate assetTemplate();
	void setAssetTemplate( const AssetTemplate & at );

signals:
	void templateChanged( const AssetTemplate & at );

public slots:
	void indexActivated( int );
	void refresh();
};
