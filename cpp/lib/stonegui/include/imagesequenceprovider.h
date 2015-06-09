
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
 * $Id: imagesequenceprovider.h 7547 2009-01-23 20:43:01Z newellm $
 */

#ifndef IMAGE_SEQUENCE_PROVIDER_H
#define IMAGE_SEQUENCE_PROVIDER_H

#include <qimage.h>
#include <qcache.h>
#include <qmutex.h>

#include "glutil.h"

class STONEGUI_EXPORT ImageSequenceProvider : public QObject
{
Q_OBJECT
public:
	ImageSequenceProvider( QObject * parent = 0 );
	virtual ~ImageSequenceProvider();

	enum ImageStatus {
		ImageNotAvailable = 0,
		ImageAvailable = 1,
		ImageCached = 2
	};

	virtual QString path() = 0;
	virtual int frameStart() = 0;
	virtual int frameEnd() = 0;

	// Providers only need to supply ImageNotFound and ImageAvailable
	// CachedImageSequenceProvider will provide ImageCached status
	virtual ImageStatus status( int frameNumber ) = 0;
	virtual QImage image( int frameNumber )=0;

signals:
	void imageStatusChange( int frameNumber, ImageStatus );
};

class STONEGUI_EXPORT CachedImageSequenceProvider : public ImageSequenceProvider
{
public:
	CachedImageSequenceProvider( ImageSequenceProvider * provider, QObject * parent = 0 );
	virtual ~CachedImageSequenceProvider();

	// In Kilobytes
	int usedCacheSize() const;
	void setMaxCacheSize( int size );

	void clearCache();

	void setImageSequenceProvider( ImageSequenceProvider * );

	virtual QString path();
	virtual int frameStart();
	virtual int frameEnd();

	virtual QImage image( int frameNumber );
	virtual ImageStatus status( int frameNumber );

protected:
	ImageSequenceProvider * mProvider;

	// Used to provide notification back to the CachedImageSequenceProvider when
	// a node is deleted from the cache.
	struct QImageCacheNode {
		QImageCacheNode( const QImage & img, CachedImageSequenceProvider * owner, int fn);
		~QImageCacheNode();
		QImage image;
		CachedImageSequenceProvider * owner;
		int frameNumber;
	};

	void notifyNodeDeleted( QImageCacheNode * );

	QCache<int,QImageCacheNode> mImageCache;
	//friend class QImageCacheNode;
};

class GLPreloadTask;
class STONEGUI_EXPORT GLImageSequenceProvider : public ImageSequenceProvider
{
public:
	GLImageSequenceProvider( QGLContext * glContext, ImageSequenceProvider * provider, QObject * parent = 0 );
	virtual ~GLImageSequenceProvider();

	// In Kilobytes
	int usedCacheSize() const;
	void setMaxCacheSize( int size );
	
	void clearCache();
	
	void setImageSequenceProvider( ImageSequenceProvider * );

	virtual QString path();
	virtual int frameStart();
	virtual int frameEnd();

	virtual QImage image( int frameNumber );

	// Valid until next textureId call, or until this provider is destroyed
	virtual GLTexture * glTexture( int frameNumber );
	virtual ImageStatus status( int frameNumber );

protected:
	QGLContext * mGLContext, * mThreadGLContext;
	QMutex mProviderMutex;
	ImageSequenceProvider * mProvider;

	// mProviderMutex must be locked before calling this function
	GLTexture * loadAndCache( int frameNumber, QGLContext * context = 0 );

	// Used to provide notification back to the CachedImageSequenceProvider when
	// a node is deleted from the cache.
	struct GLImageCacheNode {
		GLImageCacheNode( GLTexture * texture, GLImageSequenceProvider * owner, int fn );
		~GLImageCacheNode();
		GLTexture * texture;
		GLImageSequenceProvider * owner;
		int frameNumber;
	};

	void notifyNodeDeleted( GLImageCacheNode * );
	
	QCache<int,GLImageCacheNode> mGLTextureCache;
	friend class GLPreloadTask;
	int mLastFrameNumber;
	GLPreloadTask * mPreloadTask;
	
};

class STONEGUI_EXPORT ImageSequenceProviderPlugin
{
public:
	virtual ~ImageSequenceProviderPlugin(){}

	virtual QStringList fileExtensions() = 0;

	virtual bool supportsFormat( const QString & fileName = QString() ) = 0;

	virtual ImageSequenceProvider * createProvider( const QString & fileName ) = 0;
};

class STONEGUI_EXPORT ImageSequenceProviderFactory
{
public:
	static QStringList fileExtensions();

	static bool supportsFormat( const QString & fileName, ImageSequenceProviderPlugin ** plugin = 0 );
	static ImageSequenceProvider * createProvider( const QString & fileName );

	static void registerPlugin( ImageSequenceProviderPlugin * plugin );
protected:
	static QList<ImageSequenceProviderPlugin*> mPlugins;
};

void STONEGUI_EXPORT registerBuiltinImageSequenceProviderPlugins();


#endif // IMAGE_SEQUENCE_PROVIDER_H

