/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of Snafu.
 *
 * Snafu is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Snafu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Snafu; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id: snafugraphwidget.h 5408 2007-12-18 00:13:49Z brobison $
 */

#ifndef SNAFUGRAPHWIDGET_H
#define SNAFUGRAPHWIDGET_H

#include <QWidget>
#include <QImage>
#include <QMenu>
#include <QAction>

#include "snafurrdthread.h"
#include "graph.h"

class SnafuGraphWidget : public QWidget
{
	Q_OBJECT

public:
	SnafuGraphWidget(Graph graph, QWidget * parent, Qt::WindowFlags f);

public slots:
	void graphImageReady();

	void graphContextMenu(const QPoint &);
	void removeSlot();
	void editSlot();

signals:
	void removeGraph();

private:
	SnafuRrdThread *mThread;
	QImage mGraphImage;
	
	void createGraphMenu();
	QMenu * mGraphMenu;
	QAction * mRemoveGraphAct;
	QAction * mEditGraphAct;

};

#endif

