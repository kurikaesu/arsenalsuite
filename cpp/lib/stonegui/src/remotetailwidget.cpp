
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
 * $Id: remotetailwidget.cpp 6351 2008-04-17 21:07:37Z newellm $
 */

#include <qscrollbar.h>

#include "blurqt.h"
#include "multilog.h"
#include "remotelogserver.h"

#include "remotetailwidget.h"

RemoteTailWidget::RemoteTailWidget( QWidget * parent )
: QTextEdit( parent )
, mSizeRecieved( false )
, mFetchedStart( 0 )
, mFetchedEnd( 0 )
, mLastSize( 0 )
, mLogConnection( 0 )
{
	setLineWrapMode( QTextEdit::NoWrap );
	setTextInteractionFlags( Qt::TextBrowserInteraction );
	setReadOnly( true );
	mLogConnection = new RemoteLogClientConnection( this );
	connect( mLogConnection, SIGNAL( sizeRecieved( qint64 ) ), SLOT( sizeRecieved( qint64 ) ) );
	connect( mLogConnection, SIGNAL( dataRecieved( const QString &, quint64, quint32 ) ), SLOT( dataRecieved( const QString &, quint64, quint32 ) ) );
	connect( mLogConnection, SIGNAL( done( bool ) ), SLOT( connectionIdle() ) );

	connect( verticalScrollBar(), SIGNAL( valueChanged( int ) ), SLOT( verticalScrollBarValueChanged() ) );
}

RemoteTailWidget::~RemoteTailWidget()
{}

void RemoteTailWidget::connectToHost( const QHostAddress & hostAddress, int port )
{
	mLogConnection->connectToHost( hostAddress, port );
}

void RemoteTailWidget::connectToHost( const QString & hostName, int port )
{
	mLogConnection->connectToHost( hostName, port );
}

bool RemoteTailWidget::isConnected()
{
	return mLogConnection->state() != QAbstractSocket::UnconnectedState;
}

void RemoteTailWidget::setFileName( const QString & fileName )
{
	mFileName = fileName;
	mFetchedStart = mFetchedEnd = 0;
	mSizeRecieved = false;
	ensureConnectionReady();
	LOG_5( "Setting fileName: " + fileName );
	clear();
	mLogConnection->getFileSize( mFileName );
}

void RemoteTailWidget::startTailing( qint64 pos )
{
	if( ensureConnectionReady() ) {
		LOG_5( "Starting tailing file at " + QString::number( pos ) );
		mLogConnection->tailFile( mFileName, pos );
	}
}

void RemoteTailWidget::stopTailing()
{
	if( mLogConnection->currentCommand() == RemoteLogClientConnection::TailFile ) {
		LOG_5( "Cancelling tailing" );
		mLogConnection->abort();
	}
}

int RemoteTailWidget::fetchedStartPosition()
{
	return mFetchedStart;
}

int RemoteTailWidget::fetchedEndPosition()
{
	return mFetchedEnd;
}

int RemoteTailWidget::lastFileSize()
{
	return mLastSize;
}

void RemoteTailWidget::fetchPrevious( quint32 size )
{
	if( ensureConnectionReady() ) {
		mFetchStart = qMax<quint64>( 0, mFetchedStart - size );
		mFetchSize = mFetchedStart - mFetchStart;
		mTextToPrepend = QString();
		mTextToPrependSize = 0;
		if( mFetchSize > 0 ) {
			LOG_5( "Fetching previous chunk, pos: " + QString::number( mFetchStart ) + " size: " + QString::number( mFetchSize ) );
			mLogConnection->readFile( mFileName, mFetchStart, mFetchSize );
		}
	}
}

void RemoteTailWidget::fetchNext( quint32 size )
{
	if( ensureConnectionReady() ) {
		mFetchSize = qMin( size, quint32(mLastSize - mFetchedEnd) );
		mFetchStart = mFetchedEnd;
		if( mFetchSize > 0 ) {
			LOG_5( "Fetching next chunk, pos: " + QString::number( mFetchStart ) + " size: " + QString::number( mFetchSize ) );
			mLogConnection->readFile( mFileName, mFetchStart, mFetchSize );
		}
	}
}

bool RemoteTailWidget::ensureConnectionReady()
{
	if( mLogConnection->state() == QAbstractSocket::UnconnectedState ) {
		LOG_5( "Log Connection is Unconnected" );
		return false;
	}
	if( mLogConnection->currentCommand() == RemoteLogClientConnection::TailFile )
		mLogConnection->abort();
	if( mLogConnection->currentCommand() == RemoteLogClientConnection::ReadFile ) {
		LOG_5( "Log Connection is reading a file chunk" );
		return false;
	}
	return true;
}


void RemoteTailWidget::dataRecieved( const QString & text, quint64 pos, quint32 size )
{
	LOG_5( "Recieved " + QString::number( size ) + " bytes at pos: " + QString::number( pos ) );
	RemoteLogClientConnection::CommandType cc = mLogConnection->currentCommand();
	if( cc == RemoteLogClientConnection::ReadFile || cc == RemoteLogClientConnection::TailFile ) {
		if( pos == mFetchedEnd || mFetchedStart == mFetchedEnd ) {
			LOG_5( "Appending text" );
			append( text );
			if( mFetchedStart == mFetchedEnd )
				mFetchedStart = pos;
			mFetchedEnd = qMax( mFetchedEnd, pos + size );
			mLastSize = qMax( mLastSize, mFetchedEnd );
		} else {
			LOG_5( "Queueing text to prepend" );
			mTextToPrepend += text;
			mTextToPrependSize += size;
			if( mTextToPrependSize == mFetchSize ) {
				LOG_5( "Prepending text" );
				QScrollBar * vsb = verticalScrollBar();
				bool atBottom = vsb->value() == vsb->maximum();
				mFetchedStart = mFetchStart;
				QTextCursor( document() ).insertText( mTextToPrepend );
				if( atBottom )
					vsb->setValue( vsb->maximum() );
				mTextToPrepend = QString();
				mTextToPrependSize = 0;
				mFetchSize = 0;
			}
		}
	}
}

void RemoteTailWidget::sizeRecieved( qint64 size )
{
	LOG_5( "Recieved size: " + QString::number( size ) );
	mLastSize = size;
	mSizeRecieved = true;
	mFetchedStart = mFetchedEnd = size;
}

void RemoteTailWidget::connectionIdle()
{
	if( mFetchedStart == mFetchedEnd || !verticalScrollBar()->isVisible() )
		fetchPrevious( 1024 );
	else if( verticalScrollBar()->value() == verticalScrollBar()->maximum() )
		startTailing( mFetchedEnd );
}

void RemoteTailWidget::verticalScrollBarValueChanged()
{
	QScrollBar * vsb = verticalScrollBar();
	if( vsb->value() == 0 && mFetchedStart > 0 ) {
		if( mLogConnection->currentCommand() == RemoteLogClientConnection::TailFile ) // At the top
			mLogConnection->abort();
		fetchPrevious( 4096 );
	}
	else if( vsb->value() == vsb->maximum() && mLogConnection->currentCommand() != RemoteLogClientConnection::TailFile )
		startTailing( mFetchedEnd );
}

