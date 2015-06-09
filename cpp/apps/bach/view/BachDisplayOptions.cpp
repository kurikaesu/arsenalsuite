/*
 * Copyright (c) 2009 Dr. D Studios. (Please refer to license for details)
 * SVN_META_HEADURL = "$HeadURL: $"
 * SVN_META_ID = "$Id: BachDisplayOptions.cpp 9408 2010-03-03 22:35:49Z brobison $"
 */

//---------------------------------------------------------------------------------------------
#include "BachDisplayOptions.h"
#include "bachthumbnailloader.h"

static QSize sTnSize;

BachDisplayOptions::BachDisplayOptions( QWidget * a_Parent )
:	QFrame( a_Parent )
{
    setupUi( this );
    initThumbSlider();
    tnSizeChanged( mThumbSizeCombo->currentText() );
    connect( mThumbSizeCombo, SIGNAL(currentIndexChanged( const QString & )), SLOT( tnSizeChanged( const QString & ) ) );
}

void BachDisplayOptions::initThumbSlider() {
	mThumbCountSlider->setValue( 4 );
	mThumbCountSlider->setGeometry(0,0,150,20);
	mThumbCountSlider->setOrientation(Qt::Horizontal);
	mThumbCountSpin->setValue( 4 );
}

QSize BachDisplayOptions::tnSize() const
{
    return sTnSize;
}

void BachDisplayOptions::tnSizeChanged( const QString & newSize )
{
    int width = newSize.section("x", 0, 0).toInt();
    int height = newSize.section("x", 1, 1).toInt();
    sTnSize = QSize(width, height);
    ThumbnailLoader::setDiskTnSize( sTnSize );
}

