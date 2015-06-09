
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
 * $Id: imagesequencewidget.h 5411 2007-12-18 01:03:08Z brobison $
 */

#ifndef IMAGE_SEQUENCE_WIDGET_H
#define IMAGE_SEQUENCE_WIDGET_H

#include "stonegui.h"
#include "imagesequenceprovider.h"
#include "ui_imagesequencewidget.h"

#include <QTimeLine>

class STONEGUI_EXPORT ImageSequenceWidget : public QWidget, public Ui::ImageSequenceWidget
{
Q_OBJECT

public:
	ImageSequenceWidget( QWidget * parent = 0 );
	~ImageSequenceWidget();

	void setInputFile( const QString & );
	void setLoop(bool);
	void setShowControls(bool);

public slots:
	void playClicked();
	void frameChanged(int);
	void sliderMoved(int);
	void sliderPressed();
	void sliderReleased();

protected:
	ImageSequenceProvider * mProvider;
	QTimeLine * mTimeLine;
	QString mInputFile;
};

#endif // IMAGE_SEQUENCE_WIDGET_H

