
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
 * $Id: recentvaluesui.h 5411 2007-12-18 01:03:08Z brobison $
 */

#ifndef RECENT_VALUES_UI_H
#define RECENT_VALUES_UI_H

#include <qmenu.h>

#include "stonegui.h"
#include "recentvalues.h"

class STONEGUI_EXPORT RecentValuesMenu : public QMenu
{
Q_OBJECT
public:
	RecentValuesMenu( const QString & title, const RecentValues & recentValues, QWidget * parent = 0 );
	
	RecentValues recentValues() const { return mRecentValues; }

	void addRecentValue( const QVariant & value, const QString & name = QString() );

signals:
	void recentValueTriggered( const QVariant & value, const QString & name );

protected slots:
	void childActionTriggered( QAction * action );
	void refresh();

protected:

	RecentValues mRecentValues;
};

#endif // RECENT_VALUES_UI_H

