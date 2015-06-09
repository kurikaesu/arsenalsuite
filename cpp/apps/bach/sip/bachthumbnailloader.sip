
class ThumbnailLoader
{
%TypeHeaderCode
#include <bachthumbnailloader.h>
%End
public:
    ThumbnailLoader();

    static QString thumbnailCachePath( const QString & cacheDir, const QString & path, const QSize & size );
    static QString thumbnailCachePath( const QString & cacheDir, const QString & path );
    static void setDiskTnSize( const QSize & );

    static void clear( const QString & fileName );
    static QPixmap load( const QString & cacheDir, const QString & fileName, const QSize & retSize, int rotate=0, bool generate=true );
    static QImage loadImage( const BachAsset & );
    static void generate( const QString & path, const QString & cachePath, const QSize & retSize, bool cache=true );
    static QImage loadFromDisk( const QString & path );

};

