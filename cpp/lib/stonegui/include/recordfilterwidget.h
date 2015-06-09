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
 * $Id:$
 */

#ifndef RECORD_FILTER_WIDGET_H
#define RECORD_FILTER_WIDGET_H

#include <qtreeview.h>
#include <qscrollarea.h>

#include "stonegui.h"
#include "recordtreeview.h"

class QWidget;
class QGridLayout;

class STONEGUI_EXPORT RecordFilterWidget : public QScrollArea
{
Q_OBJECT

public:
    RecordFilterWidget(QWidget * parent=0);
    void setupFilters(QTreeView * mTree, const ColumnStruct columns [], IniConfig & ini);

    QMap<uint, QWidget *> mFilterMap;
    QMap<QWidget *, uint> mFilterIndexMap;
    QMap<uint, QWidget *> mTabIndexMap;

public slots:
    void resizeColumn(int, int, int);
    void moveColumn(int, int, int);
    void textFilterChanged();
    void filterRows();
    int filterChildren(const QModelIndex &);
    void clearFilters();

private:
	void setTabOrder();

    QTreeView * mTree;
    QGridLayout *layout;
    QWidget     *widget;

    QMap<QModelIndex, bool> mRowChildrenVisited;

    bool mRowFilterScheduled;
};

#endif // RECORD_FILTER_WIDGET_H

