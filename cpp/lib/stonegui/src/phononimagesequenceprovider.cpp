
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
 * $Id: ffimagesequenceprovider.cpp 5411 2007-12-18 01:03:08Z brobison $
 */

#ifdef USE_PHONON

#define __STDC_CONSTANT_MACROS

#include <QPixmap>
#include "phononimagesequenceprovider.h"

#include "blurqt.h"

class PhononImageSequenceProvider::Private
{
public:
	Private( PhononImageSequenceProvider * provider );
	~Private();

	bool open( const QString & fileName );
	bool seek( int frameNumber );

	QImage readImage( int frameNumber );
	qint64 frameNumberToTimestamp( int frameNumber );

	int mFrameStart, mFrameEnd;
	double mFrameDuration;
	
	PhononImageSequenceProvider * mProvider;
};

PhononImageSequenceProvider::Private::Private( PhononImageSequenceProvider * provider )
: mProvider( provider )
,mFrameStart(0)
,mFrameEnd(0)
{}

PhononImageSequenceProvider::Private::~Private()
{
}

bool PhononImageSequenceProvider::Private::open( const QString & path )
{
	mProvider->mMedia->setCurrentSource(path);
	mFrameStart = 0;
	if( mProvider->mMedia->hasVideo() && mProvider->mMedia->tickInterval() > 0 )
		mFrameEnd = mProvider->mMedia->totalTime() / mProvider->mMedia->tickInterval();
}

QImage PhononImageSequenceProvider::Private::readImage( int frameNumber )
{
	seek( frameNumber );
	QPixmap ret;
	mProvider->mVideo->render(&ret);
	return ret.toImage();
}

qint64 PhononImageSequenceProvider::Private::frameNumberToTimestamp( int frameNumber )
{
	// seconds = frameNumber / frameRate
	// timestamp = seconds / time_base
	// timestamp = (frameNumber / frameRate) / time_base
	// timestamp = frameNumber / (time_base * frameRate)
	//return (frameNumber * mStream->time_base.den * mStream->r_frame_rate.den) / (mStream->time_base.num * mStream->r_frame_rate.num);
	return ( frameNumber * mProvider->mMedia->tickInterval() );
}

bool PhononImageSequenceProvider::Private::seek( int frameNumber )
{
	//LOG_5( "Seeking frame " + QString::number( frameNumber ) );
	qint64 timestamp = frameNumberToTimestamp( frameNumber );
	mProvider->mMedia->seek(timestamp);
}


PhononImageSequenceProvider::PhononImageSequenceProvider( const QString & path, QObject * parent )
: ImageSequenceProvider( parent )
, d( new Private( this ) )
, mFilePath( path )
{
	mMedia = new Phonon::MediaObject();
	mMedia->setCurrentSource(path);

	mVideo = new Phonon::VideoWidget();
	Phonon::createPath(mMedia, mVideo);

	d->open( path );
}

PhononImageSequenceProvider::~PhononImageSequenceProvider()
{
	delete mMedia;
	delete mVideo;
	delete d;
}
QString PhononImageSequenceProvider::path()
{
	return mFilePath;
}

int PhononImageSequenceProvider::frameStart()
{
	return d->mFrameStart;
}

int PhononImageSequenceProvider::frameEnd()
{
	return d->mFrameEnd;
}

QImage PhononImageSequenceProvider::image( int frameNumber )
{
	return d->readImage( frameNumber );
}

ImageSequenceProvider::ImageStatus PhononImageSequenceProvider::status( int )
{
	// Assume all images in a video are available
	return ImageAvailable;
}

PhononImageSequenceProviderPlugin::PhononImageSequenceProviderPlugin()
{
}

PhononImageSequenceProviderPlugin::~PhononImageSequenceProviderPlugin(){}

QStringList PhononImageSequenceProviderPlugin::fileExtensions()
{
	QStringList ret;
	ret << "mpg" << "mpeg" << "avi" << "mov";
	LOG_5( "Supported Extensions: " + ret.join(",") );
	return ret;
}

bool PhononImageSequenceProviderPlugin::supportsFormat( const QString & fileName )
{
	mProvider = new PhononImageSequenceProvider( fileName );
	return mProvider->mMedia->hasVideo();
}


ImageSequenceProvider * PhononImageSequenceProviderPlugin::createProvider( const QString & fileName )
{
	if( mProvider ) {
		return mProvider;
	}
	return new PhononImageSequenceProvider( fileName );
}

#endif // USE_PHONON

void registerPhononImageSequenceProviderPlugin()
{
#ifdef USE_PHONON
	PhononImageSequenceProviderPlugin * plugin = new PhononImageSequenceProviderPlugin();
	plugin->fileExtensions();
	ImageSequenceProviderFactory::registerPlugin( plugin );
#endif // USE_PHONON
}

