
class ThumbnailUi
{
%TypeHeaderCode
#include <thumbnailui.h>
%End
public:
	ThumbnailUi( const Thumbnail & );

	QPixmap image( const QSize & size = QSize( 120, 120 ) ) const;
	void setImage( QPixmap toSave );
};
