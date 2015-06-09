
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

/* $Author: newellm $
 * $LastChangedDate: 2012-10-05 13:15:42 -0700 (Fri, 05 Oct 2012) $
 * $Rev: 13679 $
 * $HeadURL: svn://newellm@svn.blur.com/blur/trunk/cpp/lib/assfreezer/src/imageview.cpp $
 */

#include <qauthenticator.h>
#include <qbuffer.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qimage.h>
#include <qimagereader.h>
#include <qlayout.h>
#include <qmessagebox.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qprocess.h>
#include <qqueue.h>
#include <qregexp.h>
#include <qsslerror.h>
#include <qtimer.h>
#include <qtooltip.h>

#include "blurqt.h"
#include "freezercore.h"
#include "process.h"
#include "path.h"

#include "busywidget.h"

#include "afcommon.h"
#include "glwindow.h"
#include "imageview.h"

#ifdef USE_IMAGE_MAGICK
#include <Magick++.h>
#endif


enum {
    LOAD_IMAGE = QEvent::User
};

class LoadImageTask : public QEvent
{
public:
	LoadImageTask( QObject * rec, const QString &, int );
	
	bool run();

	QObject * reciever() const { return mReciever; }
	
	QObject * mReciever;
	bool mCancel;
	QString mPath;
	int mFrame;
	bool mExists, mApplyGamma;
	QImage mImg;
	QTime mTimer;
};

NetworkTaskManager::NetworkTaskManager()
: mNetworkAccessManager( 0 )
{
	mNetworkAccessManager = new QNetworkAccessManager(this);
	connect( mNetworkAccessManager, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)), SLOT(slotAuthRequired(QNetworkReply*,QAuthenticator*)));
	connect( mNetworkAccessManager, SIGNAL(finished(QNetworkReply*)), SLOT(slotFinished(QNetworkReply*)) );
}
	
void NetworkTaskManager::startDownload( LoadImageTask * task )
{
	IniConfig & ini = config();
	ini.pushSection( "Image_Loading" );
	QString prefix = ini.readString( "HttpUrl" );
	QString user = ini.readString( "HttpUser" );
	QString password = ini.readString( "HttpPassword" );
	ini.popSection();
	
	if( task->mPath[0].isLetter() )
		task->mPath[0] = task->mPath[0].toLower();
	
	QUrl url( prefix + task->mPath.replace(":","").replace("\\","/") );
	LOG_5( "Fetching URL: " + url.toString() );
	QNetworkRequest request(url);
	request.setRawHeader("Authorization", "Basic " + QByteArray(QString("%1:%2").arg(user).arg(password).toAscii()).toBase64()); 
	QNetworkReply * reply = mNetworkAccessManager->get(request);
	reply->setReadBufferSize(0);
	connect( reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(slotError(QNetworkReply::NetworkError)) );
#ifndef QT_NO_OPENSSL
	connect( reply, SIGNAL(sslErrors(const QList<QSslError> &)), SLOT(slotSslErrors(const QList<QSslError> &)) );
#endif
	mReplyDict[reply] = task;
}
	
void NetworkTaskManager::slotAuthRequired( QNetworkReply *, QAuthenticator * auth )
{
	//qDebug() << "Got auth required signal";
	IniConfig & ini = config();
	ini.pushSection( "Image_Loading" );
	auth->setUser( ini.readString( "HttpUser" ) );
	auth->setPassword( ini.readString( "HttpPassword" ) );
	ini.popSection();
}

void NetworkTaskManager::slotFinished( QNetworkReply * reply )
{
	//qDebug() << "Network reply finished";
	LoadImageTask * task = mReplyDict[reply];
	if( task ) {
	/*	qDebug() << "Buffer size: " << reply->bytesAvailable();
		foreach( QByteArray format, QImageReader::supportedImageFormats() )
			qDebug() << format;
		foreach( QNetworkReply::RawHeaderPair pair, reply->rawHeaderPairs() )
			qDebug() << pair.first << pair.second; */
		QString contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
		if( contentType.startsWith( "text" ) ) {
	//		qDebug() << reply->readAll();
			task->mExists = false;
		} else {
			QByteArray format, content = reply->readAll();
			QBuffer buffer(&content);
			if(contentType=="image/x-exr")
				format="exr";
			QImageReader imr(&buffer,format);
			if( imr.canRead() ) {
				task->mImg = imr.read();
				task->mExists = !task->mImg.isNull();
		//		qDebug() << "Image is null: " << task->mImg.isNull();
				task->mExists = true;
			} else {
				//qDebug() << "Can't read image";
				task->mExists = false;
			}
		}
		if( task->mReciever )
			QCoreApplication::postEvent( task->mReciever, task );
	} else
		LOG_3("Task not found for network reply");
	mReplyDict.remove(reply);
	reply->deleteLater();
}

void NetworkTaskManager::slotError(QNetworkReply::NetworkError error)
{
	LOG_3("Got network error: " + QString::number(error));
	QNetworkReply * reply = qobject_cast<QNetworkReply*>(sender());
	if( reply ) {
		LoadImageTask * task = mReplyDict[reply];
		if( task ) {
			task->mExists = false;
			if( task->mReciever )
				QCoreApplication::postEvent( task->mReciever, task );
			mReplyDict.remove(reply);
		}
		reply->deleteLater();
	}
}

#ifndef QT_NO_OPENSSL
void NetworkTaskManager::slotSslErrors(const QList<QSslError> & errors)
{
	bool onlyNoError = true;
	foreach( QSslError error, errors ) {
		LOG_3( "SSL Error: " + error.errorString() );
		if( error.errorString() != "No error" )
			onlyNoError = false;
	}
	if( QNetworkReply * reply = qobject_cast<QNetworkReply*>(sender()) ) {
		if( onlyNoError )
			reply->ignoreSslErrors();
		else  {
			reply->deleteLater();
			mReplyDict.remove(reply);
		}
	}
}
#endif

class LoadImageThread : public QThread
{
public:
	LoadImageThread();
	
	virtual void run();

	static LoadImageThread * instance();

	void addTask( LoadImageTask * );

	NetworkTaskManager * networkTaskManager();
	
protected:
	QMutex mTaskMutex;
	QWaitCondition mWait;
	NetworkTaskManager * mNetworkTaskManager;
	
	QQueue<LoadImageTask*> mImageTasks;
	bool mExit;
};

LoadImageThread::LoadImageThread()
: mNetworkTaskManager( 0 )
, mExit( false )
{

}

void LoadImageThread::run()
{
	mNetworkTaskManager = new NetworkTaskManager();
	while( !mExit ) {
		if( mImageTasks.isEmpty() ) {
			QEventLoop ev;
			ev.processEvents();
			mTaskMutex.lock();
			mWait.wait( &mTaskMutex, 10 );
			mTaskMutex.unlock();
			continue;
		}
		LoadImageTask * task = mImageTasks.dequeue();
		if( task->mCancel ) {
			delete task;
			continue;
		}
		if( !task->run() )
			delete task;
	}
}

LoadImageThread * LoadImageThread::instance()
{
	static LoadImageThread * sInstance = 0;
	if( !sInstance ) {
		sInstance = new LoadImageThread();
		sInstance->start();
	}
	return sInstance;
}

void LoadImageThread::addTask( LoadImageTask * task )
{
	mImageTasks.enqueue( task );
	mWait.wakeAll();
}

NetworkTaskManager * LoadImageThread::networkTaskManager()
{
	return mNetworkTaskManager;
}

LoadImageTask::LoadImageTask( QObject * rec, const QString & path, int frame )
: QEvent( QEvent::Type(LOAD_IMAGE) )
, mReciever( rec )
, mCancel( false )
, mPath( path )
, mFrame( frame )
{
//	mTimer.start();
}

bool LoadImageTask::run()
{
	//LOG_5( "LoadImageTask::run: Loading " + mPath );
	// do we even need to do this first, the load will fail anyways?
	/*
	if( !QFile::exists(mPath) ) {
		mExists = false;
		LOG_5( "frame does not exist: " + mPath );
		return;
	}
	*/

	IniConfig & ini = config();
	ini.pushSection( "Image_Loading" );
	QString readMode = ini.readString( "Mode", "file" );
	ini.popSection();
	
	if( mPath == LOGO_PATH || mPath.startsWith("c:", Qt::CaseInsensitive) || readMode == "file" ) {
		mExists = true;
		QByteArray format;
		if( mPath.endsWith( "exr" ) && !mApplyGamma )
			format = "exr_nogamma";
		QImageReader reader( mPath, format );
		mImg = reader.read();

	#ifdef USE_IMAGE_MAGICK
		if( mImg.isNull() ) {
			LOG_5( "QImage loading failed, trying ImageMagick" );
			mImg = ImageView::loadImageMagick(mPath);
		}
	#endif
		if( mImg.isNull() ) {
			mExists = false;
			LOG_5( "QImage loading failed: " + mPath );
			return true;
		}
		if( mReciever )
			QCoreApplication::postEvent( mReciever, this );
		return true;
	} else if ( readMode == "http" ) {
		((LoadImageThread*)QThread::currentThread())->networkTaskManager()->startDownload(this);
		return true;
	}
	return false;
	//LOG_5( "Image Loaded: " + mPath + " in " + QString::number( mTimer.elapsed() ) + "ms" );
}

static bool hasOpenGL()
{
	static bool _hasOpenGL = false, _checked = false;
	if( !_checked ) {
		_checked = true;
		_hasOpenGL = QGLFormat::hasOpenGL();
		if( _hasOpenGL )
			LOG_5( "Using OpenGL" );
	}
	return _hasOpenGL;
}

ImageView::ImageView( QWidget * parent )
: QWidget( parent )
, mShown(-1)
, mToShow(-2)
, mShowAlpha( false )
, mApplyGamma( false )
, mPlaying(false)
, mAllFramesLoaded(false)
, mGLWindow( 0 )
, mEndDigitsAreFrameNumber( true )
, mMinFrame( 0 )
, mMaxFrame( 0 )
, mLooping( true )
, mBusyWidget( 0 )
{
	if( hasOpenGL() ){
		QHBoxLayout * hbox = new QHBoxLayout(this);
		hbox->setMargin( 0 );
		mGLWindow = new GLWindow( this );
		//mGLWindow->setColorMode( GLWindow::AlphaChannel );
		hbox->addWidget( mGLWindow );
		connect( &mImageCache, SIGNAL( destroyTexInfo( const TexInfo & ) ), mGLWindow, SLOT( deleteImage( const TexInfo & ) ) );
		connect( mGLWindow, SIGNAL( scaleFactorChange( float ) ), SIGNAL( scaleFactorChange( float ) ) );
	}
	connect( &mImageCache, SIGNAL( frameStatusChange(int,int) ), SIGNAL( frameStatusChange(int,int) ) );

	setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding );
	
	mBusyWidget = new BusyWidget( this, QPixmap( "images/rotating_head.png" ) );
	mBusyWidget->move(5,5);
	mBusyWidget->hide();
}

ImageView::~ImageView()
{
	mImageCache.clear();
}

bool ImageView::event( QEvent * event )
{
	// Show the logo on the first show event, after that
	// the current image will already be shown
	if( event->type() == QEvent::Show && mToShow == -2 ) {
		setImageNumber();
	}
	return QWidget::event( event );
}

void ImageView::reset()
{
	foreach( LoadImageTask * t, mTasks )
		t->mCancel = true;
	mTasks.clear();
	mAllFramesLoaded = false;
	mImageCache.clear();
	mPlaying = false;
}

void ImageView::setFrameRange(const QString & bp, int start, int end, bool endDigitsAreFrameNumber)
{
	// Make sure it is transformed for this platform
	//mBasePath = Path( bp ).path();
	reset();

	mBasePath = Path(bp).path();
	mEndDigitsAreFrameNumber = endDigitsAreFrameNumber;
	emit gammaOptionAvailable( mBasePath.endsWith( "exr" ) );
	mMinFrame = qMin( start, end );
	mMaxFrame = qMax( start, end );
	LOG_5( "ImageView::setFrameRange: basePath=" + mBasePath + " start=" + QString::number(start) + " end=" + QString::number(end) );

	setImageNumber( -1 );
}

const QString & ImageView::basePath() const
{
	return mBasePath;
}

void ImageView::customEvent( QEvent * evt )
{
	if( evt->type() == (int)LOAD_IMAGE ) {
		LoadImageTask * lit = (LoadImageTask*)evt;
	//	LOG_5( "ImageView::customEvent: LOAD_IMAGE: path=" + lit->mPath + " frame=" + QString::number(lit->mFrame) + " total elapsed: " + QString::number( lit->mTimer.elapsed() ) );
		if( lit->mCancel ) {
			LOG_5( "ImageView::customEvent: Load Cancelled" );
			return;
		}

		if( lit->mExists ) {
			if( !lit->mImg.isNull() ) {
		//		LOG_5( "ImageView::customEvent: Loading QImage" );
				if( mGLWindow ){
					TexInfo t = mGLWindow->loadImage( &lit->mImg );
					mImageCache.addTexInfo( lit->mFrame, t );
				}else {
					mImageCache.addImage( lit->mFrame, lit->mImg );
				}
				mImageCache.setStatus( lit->mFrame, ImageCache::ImageLoaded );
			}
			if( mToShow==lit->mFrame ){
		//		LOG_5( "ImageView::customEvent: Showing image" );
				showImage();
			} //else
		//		LOG_5( "ImageView::customEvent: Not Showing Image Loaded Image: " + QString::number( lit->mFrame ) + " showing image: " + QString::number( mToShow ) );
		} else {
			LOG_5( "ImageView::customEvent: Image Not Found" );
			mImageCache.setStatus( lit->mFrame, ImageCache::ImageNotFound );
			if( mToShow == lit->mFrame ) {
				mBusyWidget->stop();
				mBusyWidget->hide();
			}
		}
		mTasks.removeAll( lit );
	}
}

bool ImageView::looping() const
{
	return mLooping;
}

void ImageView::setLooping( bool l )
{
	mLooping = l;
}

void ImageView::preloadImages()
{
	int i = mShown + 1;
	int trys = 0;
	if( mShown == -1 )
		i = mMinFrame;
	while( mTasks.size() < 1 ) {
		if( mImageCache.status( i ) == ImageCache::ImageNoInfo )
			loadImage( makeFramePath( mBasePath, i, 4, mEndDigitsAreFrameNumber ), i );
		
		trys++;
		if( trys > 3 )
			return;
		
		if( ++i > mMaxFrame ) {
			if( mShown == -1 ) { // If this is true, then we've already cycled through all the images
				mAllFramesLoaded = true;
				return;
			} else
				i = mMinFrame;
		}
		
		if( i == mShown ) {
			mAllFramesLoaded = true;
			return;
		}
	}
}

void ImageView::loadImage( const QString & path, int image )
{
	LoadImageTask * lit = new LoadImageTask( this, path, image );
	lit->mApplyGamma = mApplyGamma;
	mTasks.push_front( lit );
	LoadImageThread::instance()->addTask( lit );
	mImageCache.setStatus( image, ImageCache::ImageLoading );
}

void ImageView::setImageNumber(int num)
{
	LOG_5( "ImageView::setImageNumber: num=" + QString::number(num) );
	switch( mImageCache.status( num ) )
        {
		case ImageCache::ImageNoInfo:
		{
			QString path = (num==-1) ? QString( LOGO_PATH ) : makeFramePath( mBasePath, num, 4, mEndDigitsAreFrameNumber );
			LOG_5( "ImageView::setImageNumber: No Info, fetching " + path );
			mToShow = num;
			loadImage( path, num );
			if( !mPlaying ) {
				mBusyWidget->show();
				mBusyWidget->start();
			}
			break;
		}
		case ImageCache::ImageNotFound:
		{
			LOG_5( "ImageView::setImageNumber: Not found, Showing Logo" );
			mToShow = -1;
			showImage();
			break;
		}
		case ImageCache::ImageLoading:
		{
			LOG_5( "ImageView::setImageNumber: Waiting for image to load" );
			mToShow = num;
			if( !mPlaying ) {
				mBusyWidget->show();
				mBusyWidget->start();
			}
			break;
		}
		case ImageCache::ImageLoaded:
		{
			LOG_5( "ImageView::setImageNumber: Image Loaded, showing now" );
			mToShow = num;
			showImage();
			//preloadImages();
			break;
		}
	}
}

void ImageView::showOutputPath( int frameNumber )
{
	exploreFile( makeFramePath( mBasePath, frameNumber, 4, mEndDigitsAreFrameNumber ) );
}

void ImageView::showImage()
{
	QString path = makeFramePath(mBasePath, mToShow, 4, mEndDigitsAreFrameNumber);
	setToolTip( path );

	LOG_5( "ImageView::showImage: " + path );

	mBusyWidget->stop();
	mBusyWidget->hide();

	int status = mImageCache.status( mToShow );
	int in = (status==ImageCache::ImageLoaded) ? mToShow : -1;
	if( in == -1 && mImageCache.status( -1 ) != ImageCache::ImageLoaded )
		return;

	mShown = in;
	if( mPlaying ) {
		QTime ct = QTime::currentTime();
		int msec = mLastFrameTime.msecsTo( ct );
		ct.start();
		LOG_5( "Entering processEvents wait loop" );
		while( mLastFrameTime.isValid() && (msec + ct.elapsed()) < 30 ) {
			LOG_5( QString("Processing events for %1 ms").arg(msec + ct.elapsed()) );
			qApp->processEvents( QEventLoop::ExcludeUserInputEvents, 30 - (msec + ct.elapsed()) );
		}
		LOG_5( "Finished processEvents wait loop" );
		mLastFrameTime = ct;
	} else
		mLastFrameTime = QTime::currentTime();

	if( mGLWindow )
		mGLWindow->showImage( mImageCache.texInfo( in ) );
	else{
		mCurrentImage = mImageCache.qImage( in );
		resizeEvent(0);
		update();
	}
	if( mPlaying )
		QTimer::singleShot( 0, this, SLOT( next() ) );
}

void ImageView::next()
{
	setImageNumber( mToShow < mMaxFrame ?  mToShow+1 : mMinFrame );
}

void ImageView::prev()
{
	setImageNumber( mToShow > mMinFrame ? mToShow-1 : mMaxFrame );
}

void ImageView::play()
{
	mPlaying = true;
	next();
}

void ImageView::pause()
{
	mPlaying = false;
}

void ImageView::paintEvent(QPaintEvent * pe)
{
	if( mScaled.isNull() ) {
		QWidget::paintEvent(pe);
		return;
	}
	
	QPainter p(this);

	int x = (width() - mScaled.width())/2;
	int y = (height() - mScaled.height())/2;

	p.drawPixmap(x,y,mScaled);

	QRegion extra = pe->region()-QRegion(QRect(x,y,mScaled.width(),mScaled.height()));

	if( !extra.isEmpty() ){
		p.setClipRegion( extra );
		p.fillRect( pe->rect(), QColor(8, 5, 76) );
	}
}

void ImageView::resizeEvent(QResizeEvent *)
{
	if( mCurrentImage.isNull() )
		return;

	int w = mCurrentImage.width(), h = mCurrentImage.height();
	if( (w > width()) || (h > height()) ){
		float va = width()/(float)height();
		float pa = w/(float)h;
		if( va>pa ){
			h = height();
			w = (int)(pa*h);
		}else{
			w = width();
			h = (int)(w/pa);
		}
		mScaled = QPixmap::fromImage(mCurrentImage.scaled(w,h,Qt::KeepAspectRatio,Qt::SmoothTransformation));
	}else
		mScaled = QPixmap::fromImage(mCurrentImage);
}

void ImageView::mousePressEvent(QMouseEvent * me)
{
	if( me->button() == Qt::LeftButton ) {
		mMoveStartPos = me->globalX();
	}
}

void ImageView::mouseMoveEvent( QMouseEvent * /*me*/ )
{
#if 0
	if( me->state() & Qt::LeftButton ) {
		int frame = mToShow + me->globalX() - mMoveStartPos;
		mMoveStartPos = me->globalX();
		frame = QMIN( QMAX( frame, mMinFrame ), mMaxFrame );
		setImageNumber( frame );
	}
#endif
}

void ImageView::wheelEvent(QWheelEvent * we)
{
	float ns = scaleFactor() + (we->delta() > 0 ? .25 : -.25);
	setScaleFactor( ns < .25 ? .25 : ns );
}

void ImageView::keyPressEvent( QKeyEvent * ke )
{
	if( ke->key() == Qt::Key_Plus )
		setScaleFactor( scaleFactor() + .25 );
	else if( ke->key() == Qt::Key_Minus )
		setScaleFactor( scaleFactor() - .25 );
	else
		ke->ignore();
}

void ImageView::showAlpha( bool sa )
{
	if( mGLWindow )
		mGLWindow->setColorMode( sa ? GLWindow::AlphaChannel : GLWindow::AllChannel );
	mShowAlpha = sa;
}

void ImageView::applyGamma( bool applyGamma )
{
	if( applyGamma != mApplyGamma ) {
		mApplyGamma = applyGamma;
		reset();
		setImageNumber( mToShow );
	}
}

void ImageView::setScaleMode( int scaleMode )
{
	if( mGLWindow )
		mGLWindow->setScaleMode( scaleMode );
	emit scaleModeChange( scaleMode );
}

void ImageView::setScaleFactor( float sf )
{
	if( mGLWindow ) {
		setScaleMode( GLWindow::ScaleNone );
		mGLWindow->setScaleFactor( sf );
	}
}

float ImageView::scaleFactor() const
{
	return mGLWindow ? mGLWindow->scaleFactor() : 1.0;
}

void ImageView::destroyTexInfo( const TexInfo & texInfo )
{
	glDeleteTextures( 1, &texInfo.handle );
}

#ifdef USE_IMAGE_MAGICK
QImage ImageView::loadImageMagick( const QString & path )
{
	Magick::Image img;
	QImage ret;
	try {
		img.read(path.toStdString());

		ret = QImage(img.columns(), img.rows(), QImage::Format_RGB32);
		const Magick::PixelPacket *pixels;
		Magick::ColorRGB rgb;
		for (int y = 0; y < (int)ret.height(); y++) {
			pixels = img.getConstPixels(0, y, ret.width(), 1);
			for (int x = 0; x < ret.width(); x++) {
				rgb = (*(pixels + x));
				ret.setPixel(x, y, QColor(
					qMax(0, qMin(255, (int)(255 * rgb.red())))
					, qMax(0, qMin(255, (int)(255 * rgb.green())))
					, qMax(0, qMin(255, (int)(255 * rgb.blue())))
					).rgb()
				);
			}
		}
	} catch(...) {
		LOG_3("ImageMagick exception!");
		return ret;
	}
	return ret;
}
#endif

