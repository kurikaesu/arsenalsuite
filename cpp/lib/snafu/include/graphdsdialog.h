
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
 * $Id: graphdsdialog.h 5408 2007-12-18 00:13:49Z brobison $
 */

#ifndef GRAPHDSDIALOG_H
#define GRAPHDSDIALOG_H

#include <QDialog>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include "ui_graphdsdialogui.h"

#include "graph.h"
#include "graphds.h"
#include "graphrelationship.h"

class GraphDsDialog : public QDialog, public Ui::GraphDsDialogUI
{
Q_OBJECT

public:
    GraphDsDialog(Graph graph, QWidget * parent, Qt::WindowFlags f);
		~GraphDsDialog();

public slots:

private:
	Graph mGraph;

};

#endif

