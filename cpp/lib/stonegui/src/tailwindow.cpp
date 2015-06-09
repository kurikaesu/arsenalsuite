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
 * $Id: tailwindow.cpp 5411 2007-12-18 01:03:08Z brobison $
 */

#include <QtGui>
#include <QApplication>

#include "tailwindow.h"

/*
 * TODO
 *
 * -- proper ctor, better public interface
 * -- make file read part keep count line count rather than bytes
 * -- connect slider to MaxLines, make updateText honor maxLines
 *
 */
TailWindow::TailWindow(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	mScrollBar = ui.mText->verticalScrollBar();
	ui.mText->setReadOnly(true);

	connect(ui.mText, SIGNAL(textChanged()), this, SLOT(textChanged()));

	connect( ui.mBufferSizeSlider, SIGNAL(valueChanged(int)), this, SLOT(setBufferSizeLabelValue(int)) );
	connect( ui.mScrollButton, SIGNAL(toggled(bool)), this, SLOT(toggleStartStop(bool)) );

	mTimer = new QTimer(this);
	connect(mTimer, SIGNAL(timeout()), this, SLOT(updateText()));
}

TailWindow::readFile(void) 
{
	if ( ! mFile.atEnd() ) {
		mReadBuffer = mLogStream->readAll();
		//mAmountRead += mReadBuffer.size();
		updateText(mReadBuffer);
	}
}


TailWindow::tailFile(QString & fileName)
{
	mFile.setFileName(fileName);

	if ( mFile.open(QIODevice::ReadOnly | QIODevice::Text) ) {
	 	qint64 fileSize = mFile.size();
		mLogStream = new QTextStream(&mFile);
		if (! mFile.atEnd() ) {
			qint64 maxSeek = 5000;
			if (fileSize - maxSeek > 0) 
				mFile.seek( fileSize - maxSeek);
			while( ! mFile.atEnd() ) {
				mReadBuffer = mLogStream->readAll();
				//mAmountRead = mReadBuffer.size();
				ui.mText->insertPlainText( mReadBuffer );
				mScrollBar->setSliderPosition( mScrollBar->maximum() );
			}
		}
		mTimer->start(250);
	} else {
		ui.mText->append("file to tail not open");
	}
}

TailWindow::tailMultilog( Multilog * log ) 
{
	connect( log(), SIGNAL( logged(const QString &,int)), SLOT( slotLogged(const QString &,int)));
}

TailWindow::slotLogged(const QString & text, int level)
{
	// if ( level >= mLogLevel )	
	appendText(text)
}



TailWindow::~TailWindow() 
{
	if (mTimer) {
		mTimer->stop();
		delete mTimer;
		mTimer = 0;
	}
	if (mLogStream) {
		delete mLogStream;
		mLogStream = 0;
	}
	if (mFile.isOpen()) {
		mFile.close();
	}
}

void TailWindow::setBufferSizeLabelValue(int size)
{
	ui.mBufferSizeLabelValue->setText( QString::number(size) );
}


void TailWindow::toggleStartStop( bool isToggled )
{
	if (isToggled) {
		mTimer->stop();
		ui.mScrollButton->setText("Un-&Pause");
	} else {
		mTimer->start();
		ui.mScrollButton->setText("&Pause");
	}
}

void TailWindow::start(void)
{
	mTimer->start(250);
}

void TailWindow::stop(void)
{
	mTimer->stop();
}

void TailWindow::textChanged(void)
{
	if (atScrollEnd == true) { // set mScrollBar to maximum
		mScrollBar->setSliderPosition( mScrollBar->maximum() );
	}

}

void TailWindow::updateText(const QString & text) 
{

	if (mScrollBar->sliderPosition() != mScrollBar->maximum()) {
		atScrollEnd = false;
	} else {
		atScrollEnd = true;
	}

	QTextCursor tempcursor(ui.mText->document());
	if (atScrollEnd) {
		QString tempText = ui.mText->toPlainText();
		while(tempText.size() > 100000) { //todo
			tempcursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor, 1);
			tempcursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, 20);
			tempcursor.removeSelectedText();
			tempText = ui.mText->toPlainText();
		}
	}

		tempcursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor, 1);
		tempcursor.insertText( text );
	}

}

