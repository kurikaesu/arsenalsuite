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
 * $Id: snafuhostitem.cpp 5408 2007-12-18 00:13:49Z brobison $
 */

#include "snafuwidget.h"
#include "snafuhostitem.h"

class QTreeWidget;
class QTreeWidgetItem;

SnafuHostItem::SnafuHostItem(int type) : QTreeWidgetItem(type)
{
}

SnafuHostItem::SnafuHostItem(const QStringList & text, int type) : QTreeWidgetItem(text,type)
{
}

SnafuHostItem::SnafuHostItem(QTreeWidget * parent,int type) : QTreeWidgetItem(parent,type) { }

SnafuHostItem::SnafuHostItem(SnafuHostItem * parent,int type) : QTreeWidgetItem(parent,type) { }

SnafuHostItem::SnafuHostItem(const SnafuHostItem & other) : QTreeWidgetItem(other) { }
SnafuHostItem::SnafuHostItem(const QTreeWidgetItem & other) : QTreeWidgetItem(other) { }

void SnafuHostItem::setHost(Host host)
{
	mHost = host;
	setIcon(0, QIcon( SnafuWidget::statusPixmap(host.status()) ) );
	setText(0, host.name());
	setText(1, host.os());
	setText(2, host.description());
	setText(3, host.hostStatus().slaveStatus());
	setText(4, host.slavePulse().toString("yyyy.MM.dd hh:mm:ss"));
	//setText(5, host.xcatImageVersion);
	//setText(6, host.xcatNodeStat);
	//setText(7, QString::number(host.taskCount));
	//setText(8, QString::number(host.errorCount));
	//setText(9, MainWindow::timeCode(host.taskAverageTime));
	//setText(10, MainWindow::timeCode(host.taskSuccessTime));
}

void SnafuHostItem::setHost(int hostId)
{
	mHost = Host::recordBykey(hostId);
}

Host SnafuHostItem::host()
{
	return mHost;
}

/*
bool SnafuHostItem::operator<( const QTreeWidgetItem & other ) const
{
	int col = treeWidget()->sortColumn();
	qWarning("sortCol:"+QString::number(col).toAscii());
	switch (col) {
		case 7:
			return host().taskCount < ((SnafuHostItem)other).host().taskCount;
			break;
		case 8:
			return host().errorCount < ((SnafuHostItem)other).host().errorCount;
			break;
		case 9:
			return host().taskAverageTime < ((SnafuHostItem)other).host().taskAverageTime;
			break;
		case 10:
			return host().taskSuccessTime < ((SnafuHostItem)other).host().taskSuccessTime;
			break;

		default:
			return text(col) < other.text(col);
			break;
	}
}
*/

