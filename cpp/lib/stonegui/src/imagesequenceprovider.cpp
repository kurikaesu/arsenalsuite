
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of libstone.
 *
 * libstone is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libstone is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libstone; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id: imagesequenceprovider.cpp 9918 2010-05-26 21:52:41Z newellm $
 */

#include <qstringlist.h>
#include <qimage.h>
#include <qimagereader.h>
#include <qfileinfo.h>
#include <QGLContext>

#include "freezercore.h"
#include "path.h"

#include "imagesequenceprovider.h"

ImageSequenceProvider::ImageSequenceProvider(QObject * parent)
: QObject( parent )
{}

ImageSequenceProvider::~ImageSequenceProvider()
{}

CachedImageSequenceProvider::QImageCacheNode::QImageCacheNode( const QImage & img, CachedImageSequenceProvider * o, int fn )
: image( img )
, owner( o )
, frameNumber( fn )
{}

CachedImageSequenceProvider::QImageCacheNode::~QImageCacheNode()
{
	owner->notifyNodeDeleted(this);
}

CachedImageSequenceProvider::CachedImageSequenceProvider( ImageSequenceProvider * provider, QObject * parent )
: ImageSequenceProvider( parent )
, mProvider( 0 )
{
	// 28 Megabyte default
	setMaxCacheSize( 1024 * 28 );
	setImageSequenceProvider( provider );
}

CachedImageSequenceProvider::~CachedImageSequenceProvider()
{
	delete mProvider;
	mProvider = 0;
}

int CachedImageSequenceProvider::usedCacheSize() const
{
	return mImageCache.totalCost();
}

void CachedImageSequenceProvider::setMaxCacheSize( int size )
{
	mImageCache.setMaxCost( size );
}

void CachedImageSequenceProvider::clearCache()
{
	mImageCache.clear();
}

void CachedImageSequenceProvider::setImageSequenceProvider( ImageSequenceProvider * provider )
{
	mImageCache.clear();
	delete mProvider;
	mProvider = provider;
}

QString CachedImageSequenceProvider::path()
{
	return mProvider ? mProvider->path() : QString();
}

int CachedImageSequenceProvider::frameStart()
{
	return mProvider ? mProvider->frameStart() : 0;
}

int CachedImageSequenceProvider::frameEnd()
{
	return mProvider ? mProvider->frameEnd() : 0;
}

int imageCost( const QImage & img )
{
	return img.isNull() ? 1 : (img.width() * img.height() * 4) / 1024;
}

void CachedImageSequenceProvider::notifyNodeDeleted( QImageCacheNode * node )
{
	if( !node->image.isNull() )
		emit imageStatusChange( node->frameNumber, ImageSequenceProvider::ImageAvailable );
}

ImageSequenceProvider::ImageStatus CachedImageSequenceProvider::status( int frameNumber )
{
	QImageCacheNode * node = mImageCache.object( frameNumber );
	if( node )
		return node->image.isNull() ? ImageNotAvailable : ImageStatus(ImageAvailable | ImageCached);
	return mProvider->status( frameNumber );
}

QImage CachedImageSequenceProvider::image( int frameNumber )
{
	if( !mProvider ) return QImage();

	QImageCacheNode * node = mImageCache.object( frameNumber );
	if( node ) return node->image;

	QImage img = mProvider->image( frameNumber );

	QImageCacheNode * cn = new QImageCacheNode( img, this, frameNumber );
	emit imageStatusChange( frameNumber, img.isNull() ? ImageNotAvailable : ImageStatus(ImageAvailable | ImageCached) );

	mImageCache.insert( frameNumber, cn, imageCost( img ) );
	return img;
}

GLImageSequenceProvider::GLImageCacheNode::GLImageCacheNode( GLTexture * tex, GLImageSequenceProvider * o, int fn )
: texture( tex )
, owner( o )
, frameNumber( fn )
{}

GLImageSequenceProvider::GLImageCacheNode::~GLImageCacheNode()
{
	owner->notifyNodeDeleted(this);
	delete texture;
}

using namespace Stone;
class GLPreloadTask : public ThreadTask
{
public:
	GLPreloadTask( GLImageSequenceProvider * provider, int fn )
	: ThreadTask( QEvent::User + 1, provider )
	, mProvider( provider )
	, mFrameNumber( fn )
	{}

	void run() {
		if( !mCancel ) {
			mProvider->mProviderMutex.lock();
			//LOG_5( "Executing preload task" );
			mProvider->loadAndCache( mFrameNumber, mProvider->mThreadGLContext );
			mProvider->mPreloadTask = 0;
			mProvider->mProviderMutex.unlock();
		}
	}
	GLImageSequenceProvider * mProvider;
	int mFrameNumber;
};

GLImageSequenceProvider::GLImageSequenceProvider( QGLContext * context, ImageSequenceProvider * provider, QObject * parent )
: ImageSequenceProvider( parent )
, mGLContext( context )
, mThreadGLContext( 0 )
, mProviderMutex( QMutex::Recursive )
, mProvider( 0 )
, mPreloadTask( 0 )
{
	// 28 Megabyte default
	setMaxCacheSize( 1024 * 28 );
	setImageSequenceProvider( provider );
}

GLImageSequenceProvider::~GLImageSequenceProvider()
{
	mProviderMutex.lock();
	delete mProvider;
	mProvider = 0;
	mProviderMutex.unlock();
}

int GLImageSequenceProvider::usedCacheSize() const
{
	return mGLTextureCache.totalCost();
}

void GLImageSequenceProvider::setMaxCacheSize( int size )
{
	mGLTextureCache.setMaxCost( size );
}
	
void GLImageSequenceProvider::clearCache()
{
	mGLTextureCache.clear();
}
	
void GLImageSequenceProvider::setImageSequenceProvider( ImageSequenceProvider * provider )
{
	QMutexLocker l( &mProviderMutex );

	// Task hasn't started yet
	if( mPreloadTask ) {
		mPreloadTask->mCancel = true;
		mPreloadTask = 0;
	}

	mGLTextureCache.clear();
	delete mProvider;
	mProvider = provider;

	// Trick it into preloading if first image is fetched
	mLastFrameNumber = mProvider->frameStart() - 1;
}

QString GLImageSequenceProvider::path()
{
	QMutexLocker l( &mProviderMutex );
	return mProvider ? mProvider->path() : QString();
}

int GLImageSequenceProvider::frameStart()
{
	QMutexLocker l( &mProviderMutex );
	return mProvider ? mProvider->frameStart() : 0;
}

int GLImageSequenceProvider::frameEnd()
{
	QMutexLocker l( &mProviderMutex );
	return mProvider ? mProvider->frameEnd() : 0;
}

QImage GLImageSequenceProvider::image( int frameNumber )
{
	QMutexLocker l( &mProviderMutex );
	return mProvider ? mProvider->image( frameNumber ) : QImage();
}

void GLImageSequenceProvider::notifyNodeDeleted( GLImageCacheNode * node )
{
	if( node->texture )
		emit imageStatusChange( node->frameNumber, ImageSequenceProvider::ImageAvailable );
}

ImageSequenceProvider::ImageStatus GLImageSequenceProvider::status( int frameNumber )
{
	GLImageCacheNode * node = mGLTextureCache.object( frameNumber );
	if( node )
		return node->texture ? ImageStatus(ImageAvailable | ImageCached) : ImageNotAvailable;
	return mProvider->status( frameNumber );
}

GLTexture * GLImageSequenceProvider::loadAndCache( int frameNumber, QGLContext * context )
{
	QImage img = image( frameNumber );

	if( !context ) context = mGLContext;

	GLImageCacheNode * node = new GLImageCacheNode( img.isNull() ? 0 : new GLTexture( context, img ), this, frameNumber );
	emit imageStatusChange( frameNumber, img.isNull() ? ImageNotAvailable : ImageStatus(ImageAvailable | ImageCached) );

	//LOG_5( "Inserting image into cache with cost: " + QString::number( imageCost( img ) / 1024 ) + "Mb" );
	mGLTextureCache.insert( frameNumber, node, imageCost( img ) );
	return node->texture;
}

GLTexture * GLImageSequenceProvider::glTexture( int frameNumber )
{
	GLTexture * ret = 0;
	if( !mProvider ) return ret;

	//LOG_5( "Frame " + QString::number( frameNumber ) + " requested, Current Cache Size: " + QString::number( mGLTextureCache.totalCost() / 1024 ) + "Mb" );

	// Lock mutex, to ensure any threads loading a texture will either be completely done or not started
	QMutexLocker l( &mProviderMutex );

	GLImageCacheNode * node = mGLTextureCache.object( frameNumber );
	if( node ) {
		//LOG_5( "Found Image in Cache, returning" );
		ret = node->texture;
	} else
		ret = loadAndCache( frameNumber );

	if( !mPreloadTask ) {
		if( frameNumber < frameEnd() ) {
			if( frameNumber == mLastFrameNumber + 1 ) {
				if( !mGLTextureCache.object( frameNumber + 1 ) ) {
					//LOG_5( "Starting preload task" );
					if( !mThreadGLContext ) {
						mThreadGLContext = new QGLContext( mGLContext->format(), mGLContext->device() );
						mThreadGLContext->create( mGLContext );
					}
					mPreloadTask = new GLPreloadTask( this, frameNumber + 1 );
					FreezerCore::addTask( mPreloadTask );
				} else
					LOG_5( "Not preloading, next frame already cached" );
			} else
				LOG_5( "Not preloading, frame is not sequential from last" );
		} else
			LOG_5( "Not preloading, frame is last in series" );
	} else
		LOG_5( "Not preloading, existing preload in progress" );

	mLastFrameNumber = frameNumber;
	return ret;
}

QList<ImageSequenceProviderPlugin*> ImageSequenceProviderFactory::mPlugins;

QStringList ImageSequenceProviderFactory::fileExtensions()
{
	QStringList ret;
	foreach( ImageSequenceProviderPlugin * plugin, mPlugins )
		ret += plugin->fileExtensions();
	return ret;
}

bool ImageSequenceProviderFactory::supportsFormat( const QString & fileName, ImageSequenceProviderPlugin ** pluginRet )
{
	foreach( ImageSequenceProviderPlugin * plugin, mPlugins )
		if( plugin->supportsFormat( fileName ) ) {
			if( pluginRet )
				*pluginRet = plugin;
			return true;
		}
	return false;
}

ImageSequenceProvider * ImageSequenceProviderFactory::createProvider( const QString & fileName )
{
	ImageSequenceProviderPlugin * plug = 0;
	if( supportsFormat( fileName, &plug ) )
		return plug->createProvider( fileName );
	return 0;
}

void ImageSequenceProviderFactory::registerPlugin( ImageSequenceProviderPlugin * plugin )
{
	mPlugins += plugin;
}

class FrameListImageSequenceProvider : public ImageSequenceProvider
{
public:
	FrameListImageSequenceProvider( const QString imagePath )
	: mFrameStart( 0 )
	, mFrameEnd( 0 )
	{
		mBasePath = Path(imagePath).dirPath() + framePathBaseName( imagePath );
		QList<int> frames;
		filesFromFramePath( mBasePath, &frames, &mPadWidth );
		if( !frames.isEmpty() ) {
			mFrameStart = mFrameEnd = frames[0];
			foreach( int frame, frames ) {
				mFrameStart = qMin( mFrameStart, frame );
				mFrameEnd = qMax( mFrameEnd, frame );
			}
		}
		LOG_5( "Found image sequence at basePath: " + mBasePath + " frame range: " + QString::number( mFrameStart ) + " - " + QString::number( mFrameEnd ) );
	}

	int frameStart()
	{ return mFrameStart; }

	int frameEnd()
	{ return mFrameEnd; }

	QImage image( int frameNumber )
	{
		return QImage( makeFramePath( mBasePath, frameNumber, mPadWidth ) );
	}

	ImageStatus status( int frameNumber )
	{ return QFileInfo( makeFramePath( mBasePath, frameNumber, mPadWidth ) ).exists() ? ImageAvailable : ImageNotAvailable; }

	QString path()
	{ return mBasePath; }

protected:
	int mFrameStart, mFrameEnd, mPadWidth;
	QString mBasePath;
};

class BuiltinImageSequenceProvidersPlugin : public ImageSequenceProviderPlugin
{
	QStringList fileExtensions()
	{
		QList<QByteArray> imgFormats = QImageReader::supportedImageFormats();
		QStringList ret;
		foreach( QByteArray ba, imgFormats )
			ret += QString::fromLatin1( ba.constData() );
		return ret;
	}

	bool supportsFormat( const QString & fileName = QString() )
	{
		return !QImageReader::imageFormat(fileName).isEmpty();
	}

	ImageSequenceProvider * createProvider( const QString & fileName )
	{
		return new FrameListImageSequenceProvider( fileName );
	}
};

void registerBuiltinImageSequenceProviderPlugins()
{
	ImageSequenceProviderFactory::registerPlugin( new BuiltinImageSequenceProvidersPlugin() );
}

