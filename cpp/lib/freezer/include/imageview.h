
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of Arsenal.
 *
 * Arsenal is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Arsenal is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Blur; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* $Author$
 * $LastChangedDate: 2009-11-23 11:30:56 +1100 (Mon, 23 Nov 2009) $
 * $Rev: 9055 $
 * $HeadURL: svn://svn.blur.com/blur/branches/concurrent_burn/cpp/lib/assfreezer/include/items.h $
 */

#ifndef IMAGE_VIEW_H
#define IMAGE_VIEW_H

#include <qwidget.h>
#include <qdatetime.h>
#include <qlist.h>
#include <qpixmap.h>
#include <qmap.h>
#include <qnetworkreply.h>

#include "imagecache.h"

#include "afcommon.h"

class QImage;
class QNetworkAccessManager;
class QAuthenticator;

class GLWindow;
class LTGA;
class LoadImageTask;
class BusyWidget;

class NetworkTaskManager : public QObject
{
Q_OBJECT
public:
	NetworkTaskManager();
	
	void startDownload( LoadImageTask * );
	
public slots:
	void slotAuthRequired( QNetworkReply *, QAuthenticator * );
	void slotFinished( QNetworkReply * );
	void slotError(QNetworkReply::NetworkError);
#ifndef QT_NO_OPENSSL
	void slotSslErrors(const QList<QSslError> & errors);
#endif

protected:
	QNetworkAccessManager * mNetworkAccessManager;
	QMap<QNetworkReply*, LoadImageTask*> mReplyDict;
};

class FREEZER_EXPORT ImageView : public QWidget
{
Q_OBJECT
public:
	ImageView(QWidget * parent=0);
	~ImageView();
	
	// Setup
	void setFrameRange(const QString & basePath, int start, int end, bool endDigitsAreFrameNumber );
	const QString & basePath() const;

	bool looping() const;

	float scaleFactor() const;
#ifdef USE_IMAGE_MAGICK
	static QImage loadImageMagick(const QString &);
#endif

public slots:

	// Navigation
	void setImageNumber(int in=-1);
	void showOutputPath(int fn=-1);
	void next();
	void prev();
	void play();
	void pause();

	void showAlpha( bool );
	void applyGamma( bool );
	void setScaleMode( int );
	void setScaleFactor( float );
	
	void destroyTexInfo( const TexInfo & );
	void setLooping( bool );

//	int cacheStatus( int fn );

signals:
	void scaleFactorChange( float sf );
	void scaleModeChange( int );
	void frameStatusChange(int,int);
	void gammaOptionAvailable(bool);

protected:
	void reset();
	void customEvent( QEvent * );
	bool event( QEvent * );
	void showImage();
	virtual void paintEvent(QPaintEvent *);
	virtual void mousePressEvent(QMouseEvent *);
	virtual void mouseMoveEvent(QMouseEvent *);
	virtual void resizeEvent(QResizeEvent *);
	virtual void wheelEvent(QWheelEvent *);
	virtual void keyPressEvent( QKeyEvent * );

	void preloadImages();
	void loadImage( const QString & path, int image );
private:

	QString mBasePath;

	int mShown, mToShow;

	QImage mCurrentImage;
	QPixmap mScaled;
	
	bool mShowAlpha;
	bool mApplyGamma;
	bool mPlaying;
	bool mAllFramesLoaded;

	GLWindow * mGLWindow;

	ImageCache mImageCache;
	QTime mLastFrameTime;

	bool mEndDigitsAreFrameNumber;
	int mMinFrame, mMaxFrame;

	bool mLooping;

	// Mark the globalX pos on last most press/move
	int mMoveStartPos;

	QList<LoadImageTask*> mTasks;

	BusyWidget * mBusyWidget;
};


#endif // IMAGE_VIEW_H

