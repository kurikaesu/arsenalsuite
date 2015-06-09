
#ifndef THUMBNAIL_UI_H
#define THUMBNAIL_UI_H

#include "thumbnail.h"

#include "classesui.h"

class CLASSESUI_EXPORT ThumbnailUi
{
public:
	ThumbnailUi( const Thumbnail & );

	QPixmap image( const QSize & size = QSize( 120, 120 ) ) const;
	void setImage( QPixmap toSave );
protected:
	Thumbnail mThumbnail;
};

#endif // THUMBNAIL_UI_H

