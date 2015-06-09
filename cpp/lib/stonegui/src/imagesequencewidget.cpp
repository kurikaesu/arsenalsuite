
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
 * $Id: imagesequencewidget.cpp 5411 2007-12-18 01:03:08Z brobison $
 */

#include <qpixmap.h>
#include <qslider.h>
#include <qpushbutton.h>

#include "imagesequencewidget.h"

ImageSequenceWidget::ImageSequenceWidget( QWidget * parent )
: QWidget( parent )
{
	setupUi(this);
	mTimeLine = new QTimeLine(1000,this);
	connect( mPlayButton, SIGNAL( clicked() ), this, SLOT( playClicked() ) );
	connect( mSlider, SIGNAL( sliderMoved(int) ), this, SLOT( sliderMoved(int) ) );
	connect( mSlider, SIGNAL( sliderPressed() ), this, SLOT( sliderPressed() ) );
	connect( mTimeLine, SIGNAL( frameChanged(int) ), this, SLOT( frameChanged(int) ) );
}

ImageSequenceWidget::~ImageSequenceWidget()
{
}

void ImageSequenceWidget::setInputFile( const QString & path )
{
	mProvider = ImageSequenceProviderFactory::createProvider( path );
	mTimeLine->setFrameRange( mProvider->frameStart(), mProvider->frameEnd() );
	mSlider->setRange( mProvider->frameStart(), mProvider->frameEnd() );
}

void ImageSequenceWidget::playClicked()
{
	if( mTimeLine->state() == QTimeLine::Running ) {
		mTimeLine->setPaused(true);
		mPlayButton->setIcon(QIcon("images/player_pause.png"));
	} else {
		mPlayButton->setIcon(QIcon("images/player_play.png"));
		mTimeLine->start();
	}
}

void ImageSequenceWidget::sliderMoved(int curVal)
{
	mTimeLine->setCurrentTime( curVal );
}

void ImageSequenceWidget::sliderPressed()
{
	mTimeLine->setPaused(true);
	mPlayButton->setIcon(QIcon("images/player_pause.png"));
}

void ImageSequenceWidget::sliderReleased()
{
}

void ImageSequenceWidget::frameChanged(int frame)
{
	QImage scaled = mProvider->image(frame).scaled(mImageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);;
	mImageLabel->setPixmap( QPixmap::fromImage( scaled ) );
	mSlider->setValue( frame );
}

void ImageSequenceWidget::setShowControls(bool show)
{
	if(show) {
		mSlider->show();
		mPlayButton->show();
	} else {
		mSlider->hide();
		mPlayButton->hide();
	}
}

void ImageSequenceWidget::setLoop(bool loop)
{
	if(loop)
		mTimeLine->setLoopCount(0);
	else
		mTimeLine->setLoopCount(1);
}

