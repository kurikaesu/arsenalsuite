
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
 * $Id: snafugraphwidget.cpp 5408 2007-12-18 00:13:49Z brobison $
 */

#include "snafugraphwidget.h"
#include "snafurrdthread.h"

SnafuGraphWidget::SnafuGraphWidget(Graph graph, QWidget * parent = 0, Qt::WindowFlags f = 0) : QWidget(parent, f)
{
	createGraphMenu();
  connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(graphContextMenu(const QPoint &)));

	mThread = new SnafuRrdThread(graph, parent);
	connect(mThread, SIGNAL(graphReady()), this, SLOT(graphImageReady()));
	mThread->start();
}

void SnafuGraphWidget::graphImageReady()
{
	mGraphImage = mThread->graphImage();
}

void SnafuGraphWidget::graphContextMenu( const QPoint & point )
{
	Q_UNUSED(point);
	QAction * result = mGraphMenu->exec( QCursor::pos() );
	if (result)
	{
	 // anything??
	}
}

void SnafuGraphWidget::createGraphMenu()
{
  mRemoveGraphAct = new QAction(QIcon("images/vnc.bmp"), tr("Remove"), this);
  mRemoveGraphAct->setStatusTip(tr("Remove graph from view"));
	mRemoveGraphAct->setData(QVariant("remove_graph"));
  connect(mRemoveGraphAct, SIGNAL(triggered()), this, SLOT(removeSlot()));

  mEditGraphAct = new QAction(QIcon("images/vnc.bmp"), tr("Edit"), this);
  mEditGraphAct->setStatusTip(tr("Edit graph"));
	mEditGraphAct->setData(QVariant("edit_graph"));
  connect(mEditGraphAct, SIGNAL(triggered()), this, SLOT(editSlot()));

	mGraphMenu = new QMenu("Graph");
	mGraphMenu->addAction(mRemoveGraphAct);
	mGraphMenu->addAction(mEditGraphAct);
}

void SnafuGraphWidget::removeSlot()
{
	emit removeGraph();
}

void SnafuGraphWidget::editSlot()
{
}

