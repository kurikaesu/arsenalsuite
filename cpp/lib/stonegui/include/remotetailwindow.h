
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
 * $Id: remotetailwindow.h 6351 2008-04-17 21:07:37Z newellm $
 */

#ifndef REMOTE_TAIL_WINDOW_H
#define REMOTE_TAIL_WINDOW_H

#include "ui_remotetailwindowui.h"

#include "stonegui.h"

class STONEGUI_EXPORT RemoteTailWindow : public QMainWindow, public Ui::RemoteTailWindowUI
{
Q_OBJECT
public:
	RemoteTailWindow( QWidget * parent = 0 );
	~RemoteTailWindow();

	RemoteTailWidget * tailWidget() { return mTailWidget; }

public slots:
	/// Name of service, only used to display in title bar
	void setServiceName( const QString & );

protected slots:
	void remoteHostChanged();

protected:
	QString mServiceName;
};


#endif // REMOTE_TAIL_WINDOW_H

