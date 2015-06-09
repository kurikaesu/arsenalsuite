

#include <qfile.h>
#include <qpixmap.h>

#include "path.h"

#include "thumbnailui.h"

ThumbnailUi::ThumbnailUi( const Thumbnail & t )
: mThumbnail( t )
{}

QPixmap ThumbnailUi::image( const QSize & retSize ) const
{
	if( QFile::exists( mThumbnail.filePath() ) ) {
	//	QPixmap pix = ThumbnailLoader::load( filePath(), retSize );
	//	if( !pix.isNull() )
	//		return pix;
		return QPixmap( mThumbnail.filePath() );
	}
	return QPixmap();
}
	
void ThumbnailUi::setImage( QPixmap toSave )
{
	Path savePath( mThumbnail.filePath() );
	if( savePath.dir().mkdir() ) {
		toSave.save( savePath.path(), "PNG" );
	//	ThumbnailLoader::clear( savePath.path() );
	}
	mThumbnail.setDate( QDateTime::currentDateTime() );
}
