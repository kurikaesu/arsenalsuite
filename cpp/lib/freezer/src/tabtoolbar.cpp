
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
 * $LastChangedDate: 2009-12-02 15:20:53 -0800 (Wed, 02 Dec 2009) $
 * $Rev: 9126 $
 * $HeadURL: svn://newellm@ocelot/blur/trunk/cpp/lib/assfreezer/src/tabtoolbar.cpp $
 */


#include <qpushbutton.h>
#include <qtabwidget.h>
#include <qtabbar.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qlineedit.h>
#include <QKeyEvent>

#include "tabtoolbar.h"
#include "afcommon.h"
#include "glwindow.h"
#include "imageview.h"

class TabWidgetHack : public QTabWidget
{
public:
	QTabBar * tabBarHack(){
		return tabBar();
	}
};

int tabBarHeight( QTabWidget * tabWidget )
{
	return ((TabWidgetHack*)tabWidget)->tabBarHack()->height();
}

TabToolBar::TabToolBar(QTabWidget * parent, ImageView * iv )
: QWidget( parent )
, mImageView( iv )
{
	mAlpha = new QPushButton( QIcon( ":/images/show_alpha.png" ),"", this );
	mAlpha->setCheckable( true );
	mAlpha->setToolTip( "Show Alpha Channel" );
	mAlpha->setMaximumSize(22, 22);

	mPlay = new QPushButton( QIcon( ":/images/run.png" ), "", this );
	mPlay->setToolTip( "Play Frames" );
	mPlay->setMaximumSize(22, 22);

	mPause = new QPushButton( QIcon( ":/images/pause.png" ), "", this );
	mPause->setEnabled( false );
	mPause->setToolTip( "Pause Frames" );
	mPause->setMaximumSize(22, 22);

	QWidget * spacer = new QWidget();
	spacer->setMaximumWidth(10);

	QWidget * spacer2 = new QWidget();
	spacer2->setMaximumWidth(10);

	QLabel * scaleLabel = new QLabel( "Scale:", this );
	scaleLabel->setMaximumWidth(50);

	mScaleCombo = new QComboBox( this );
	mScaleCombo->setEditable( true );
//	mScaleCombo->setInsertionPolicy( QComboBox::NoInsertion );
	mScaleCombo->installEventFilter( this );
	mScaleCombo->lineEdit()->installEventFilter( this );
	mScaleCombo->setMaximumWidth(60);

	mScaleCombo->addItem( "0.5" );
	mScaleCombo->addItem( "1.0" );
	mScaleCombo->addItem( "1.5" );
	mScaleCombo->addItem( "2.0" );

	mFreeScaleCheck = new QCheckBox( "Free Scale", this );
	mFreeScaleCheck->setChecked( true );

	mGamma = new QPushButton( QIcon(":/images/show_gamma.png"), "", this );
	mGamma->setCheckable( true );
	mGamma->setChecked( false );
	mGamma->setToolTip( "Apply Screen Gamma Correction" );
	mGamma->hide();

	connect( mAlpha, SIGNAL( toggled( bool ) ), mImageView, SLOT( showAlpha( bool ) ) );
	connect( mPlay, SIGNAL( clicked() ), SLOT( slotPlay() ) );
	connect( mPause, SIGNAL( clicked() ), SLOT( slotPause() ) );
	connect( mScaleCombo, SIGNAL( activated( const QString & ) ), SLOT( slotScaleComboChange( const QString & ) ) );
	connect( mFreeScaleCheck, SIGNAL( toggled( bool ) ), SLOT( slotFreeScaleToggled( bool ) ) );

	QHBoxLayout *controlsLayout = new QHBoxLayout(this);
	controlsLayout->setSpacing(1);
	controlsLayout->setMargin(0);
	controlsLayout->addWidget(mAlpha);
	controlsLayout->addWidget(mGamma);
	controlsLayout->addWidget(mPlay);
	controlsLayout->addWidget(mPause);
	controlsLayout->addWidget(scaleLabel);
	controlsLayout->addWidget(mScaleCombo);
	controlsLayout->addWidget(mFreeScaleCheck);
	connect( mImageView, SIGNAL( scaleFactorChange( float ) ), SLOT( slotSetScaleFactor( float ) ) );
	connect( mImageView, SIGNAL( scaleModeChange( int ) ), SLOT( slotScaleModeChanged( int ) ) );
	connect( mImageView, SIGNAL( gammaOptionAvailable( bool ) ), mGamma, SLOT( setVisible( bool ) ) );
	connect( mGamma, SIGNAL( toggled( bool ) ), mImageView, SLOT( applyGamma( bool ) ) );

#ifdef Q_WS_MAC
	if( qApp->style()->inherits("QMacStyle") ) {
		mAlpha->setFixedHeight(20);
		mPlay->setFixedHeight(20);
		mPause->setFixedHeight(20);
		setGeometry( 140, -5, controlsLayout->sizeHint().width(), controlsLayout->sizeHint().height()-2 );
	} else {
		setGeometry( 400, 0, controlsLayout->sizeHint().width(), controlsLayout->sizeHint().height()-4 );
	}
#else
	setGeometry( 400, 0, controlsLayout->sizeHint().width(), controlsLayout->sizeHint().height()-4 );
#endif
}

void TabToolBar::slotPlay()
{
	mPause->setEnabled( true );
	mPlay->setEnabled( false );
	mImageView->play();
}

void TabToolBar::slotPause()
{
	mPause->setEnabled( false );
	mPlay->setEnabled( true );
	mImageView->pause();
}

bool TabToolBar::eventFilter( QObject * o, QEvent * ev )
{
	bool setScale = false;
	float newFactor = mImageView->scaleFactor();
		
	if( o == mScaleCombo->lineEdit() && ev->type() == QEvent::KeyPress ) {
		QKeyEvent * ke = (QKeyEvent*)ev;
		if( ke->key() == Qt::Key_Plus ) {
			setScale = true;
			newFactor += .25;
			ke->accept();
		} else if( ke->key() == Qt::Key_Minus ) {
			setScale = true;
			newFactor -= .25;
			ke->accept();
		}
	}
	if( o == mScaleCombo && ev->type() == QEvent::Wheel ) {
		QWheelEvent * we = (QWheelEvent*)ev;
		setScale = true;
		newFactor = newFactor + ( we->delta() > 0 ? .25 : -.25 );
		we->accept();
	}
	if( setScale ) {
		mImageView->setScaleFactor( newFactor >= .25 ? newFactor : .25  );
		return true;
	}
	return QWidget::eventFilter( o, ev );
}

void TabToolBar::slotScaleComboChange( const QString & text )
{
	bool valid = false;
	float scale = text.toFloat( &valid );
	if( valid ) {
		mFreeScaleCheck->setChecked( false );
		mImageView->setScaleFactor( scale );
	}
}

void TabToolBar::slotFreeScaleToggled( bool fs )
{
	mImageView->setScaleMode( fs ? GLWindow::ScaleAlways : GLWindow::ScaleNone );
}

void TabToolBar::slotSetScaleFactor( float sf )
{
	mScaleCombo->blockSignals( true );
	mScaleCombo->setEditText( QString::number( sf, 'f', 2 ) );
	mScaleCombo->blockSignals( false );
}

void TabToolBar::slotScaleModeChanged( int sm )
{
	mFreeScaleCheck->blockSignals( true );
	mFreeScaleCheck->setChecked( sm == GLWindow::ScaleAlways );
	mFreeScaleCheck->blockSignals( false );
}


