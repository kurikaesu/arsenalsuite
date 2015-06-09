
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
 * $Id: items.cpp 9407 2010-03-03 22:17:59Z brobison $
 */

#include <qpixmap.h>
#include <qpainter.h>
#include <qdatetime.h>
#include <qheaderview.h>
#include <qpixmapcache.h>

#include "items.h"
#include "iniconfig.h"
#include "project.h"
#include "employee.h"
#include "jobtype.h"
#include "blurqt.h"
#include "snafuwidget.h"

static const ColumnStruct host_columns [] =
{
	{ "Host", "HostColumn", 100, 0, false },
	{ "Current Job", "CurrentJobColumn", 200, 1, false },
	{ "Status", "StatusColumn", 50, 2, false },
	{ "Frames", "FramesColumn", 50, 3, false },
	{ "OS", "OSColumn", 40, 4, false },
	{ "Memory", "MemoryColumn", 30, 5, false },
	{ "Mhz", "MhzColumn", 30, 6, false },
	{ "User", "UserColumn", 60, 7, false },
	{ "Packet Weight", "PacketWeightColumn", 40, 8, true },
	{ "Description", "DescriptionColumn", 200, 9, false },
	{ "Pulse", "PulseColumn", 50, 10, false },
	{ "Key", "KeyColumn", 0, 11, true },
	{ 0, 0, 0, 0, false }
};

void setupHostView( RecordTreeView * lv )
{
	IniConfig & cfg = userConfig();
	cfg.pushSubSection( "HostList" );
	lv->setupTreeView(cfg,host_columns);
	cfg.popSection();
}

void saveHostView( RecordTreeView * lv )
{
	IniConfig & cfg = userConfig();
	cfg.pushSubSection( "HostList" );
	lv->saveTreeView(cfg,host_columns);
	cfg.popSection();
}

void drawGrad( QPainter * p, QColor c, int x, int y, int w, int h )
{
	c = c.light(130);
	for( ; h>0; h-- ){
		p->setPen( c );
		p->drawLine( x, y+h-1, x+w, y+h-1 );
		c = c.dark( 105 );
	}
}

QSize MultiLineDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	const QAbstractItemModel *model = index.model();
	QVariant value = model->data(index, Qt::FontRole);
	QFont fnt = value.isValid() ? qvariant_cast<QFont>(value) : option.font;
	QString text = model->data(index, Qt::DisplayRole).toString().trimmed();
	QFontMetrics fontMetrics(fnt);
	return fontMetrics.size( 0, text );
}

QVariant civ( const QColor & c )
{
	if( c.isValid() )
		return QVariant(c);
	return QVariant();
}

void HostItem::setup( const Record & r, const QModelIndex & ) {
	host = r;
	hostStatus = host.hostStatus();
	jobName = QString(); //hostStatus.job().name();
	ver = host.os() + " " + host.abVersion();
	mem = QString("%1 Mb").arg(host.memory());
	mhz = QString("%1 Mhz").arg(host.mhz() * host.cpus());
	user = Employee::recordByHost( host ).name();
	pulse = hostStatus.slavePulse().toString(Qt::ISODate);
	//co = options.mHostColors->getColorOption(status.slaveStatus());
}

QVariant HostItem::modelData( const QModelIndex & i, int role ) const {
	int col = i.column();
	if( role == Qt::DisplayRole ) {
		switch( col ) {
			case 0: return host.name();
			case 1: return jobName;
			case 2: return hostStatus.slaveStatus();
			case 3: return QString(); //hostStatus.slaveFrames();
			case 4: return ver;
			case 5: return mem;
			case 6: return mhz;
			case 7: return user;
			case 8: return "";
			case 9: return host.description();
			case 10: return pulse;
			case 11: return host.key();
		}
	} else if ( col == 0 && role == Qt::DecorationRole ) {
		QPixmap result = QPixmap(":/images/clear.png");
		switch (Host(host).syslogStatus()) {
			case 0:
				QPixmapCache::find("host-status-clear", result);
				break;
			case 1:
				QPixmapCache::find("host-status-warning", result);
				break;
			case 2:
				QPixmapCache::find("host-status-minor", result);
				break;
			case 3:
				QPixmapCache::find("host-status-major", result);
				break;
			case 4:
				QPixmapCache::find("host-status-critical", result);
				break;
		}
		return QVariant(result);
	} /* else if ( role == Qt::TextColorRole )
		return co ? civ(co->fg) : QVariant();
	else if( role == Qt::BackgroundColorRole )
		return co ? civ(co->bg) : QVariant();
		*/
	return QVariant();
}

char HostItem::getSortChar() const {
	QString stat = hostStatus.slaveStatus();
	if( stat=="ready" ) return 'a';
	else if( stat=="assigned" ) return 'b';
	else if( stat=="copy" ) return 'c';
	else if( stat=="busy" ) return 'd';
	else if( stat=="starting" ) return 'e';
	else if( stat=="waking" ) return 'f';
	else if( stat=="sleep" ) return 'g';
	else if( stat=="sleeping" ) return 'h';
	else if( stat=="client-update" ) return 'i';
	else if( stat=="restart" ) return 'j';
	else if( stat=="stopping" ) return 'k';
	else if( stat=="no-ping" ) return 'l';
	else if( stat=="offline" ) return 'm';
	else return 'n';
}

int HostItem::compare( const QModelIndex & a, const QModelIndex & b, int column, bool asc )
{
	HostItem & other = HostModel::data(b);
	if( column == 2 ) {
		char sca = getSortChar();
		char scb = other.getSortChar();
		return sca == scb ? 0 : (sca > scb ? 1 : -1);
	}
	return ItemBase::compare(a,b,column,asc);
}

Qt::ItemFlags HostItem::modelFlags( const QModelIndex & ) { return Qt::ItemFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled ); }
Record HostItem::getRecord() { return host; }

