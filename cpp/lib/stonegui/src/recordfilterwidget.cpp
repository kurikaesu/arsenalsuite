
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

#include <qlayout.h>
#include <qwidget.h>
#include <qgridlayout.h>
#include <qboxlayout.h>
#include <qscrollarea.h>
#include <qscrollbar.h>
#include <qlineedit.h>
#include <qtimer.h>
#include <qheaderview.h>

#include "iniconfig.h"
#include "recordfilterwidget.h"
#include "recordtreeview.h"

RecordFilterWidget::RecordFilterWidget(QWidget * parent)
: QScrollArea(parent)
, mRowFilterScheduled(false)
{
    setMaximumHeight(20);

    widget = new QWidget(this);
    layout = new QGridLayout(widget);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    setWidget(widget);
    setWidgetResizable(true);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void RecordFilterWidget::setupFilters(QTreeView * tree, const ColumnStruct columns [], IniConfig & ini)
{
    mTree = tree;

	int i;
	for (i = 0; i < mTree->header()->count(); i++) {

		int visIndex = mTree->header()->visualIndex(i);

		if (columns[i].filterEnabled) {

			QLineEdit * edit = new QLineEdit(this);
			edit->setMaximumHeight(18);
			edit->setToolTip(QString("Filter by: %1").arg(columns[i].name));

			QString filterText = ini.readString( columns[i].iniName + QString("ColumnFilter"), "" );
			if( !filterText.isEmpty() ) {
				edit->setText( filterText );
				SuperModel * sm = (SuperModel *)(tree->model());
				sm->setColumnFilter( i, filterText );
			}

			mFilterMap[i] = edit;

			connect(mFilterMap[i], SIGNAL(textChanged(const QString)), this, SLOT(textFilterChanged()));
		} else {
			mFilterMap[i] = new QWidget();
		}

		mFilterMap[i]->setFixedWidth( mTree->columnWidth( i ) );
		layout->addWidget(mFilterMap[i], 0, visIndex);

		mFilterIndexMap[mFilterMap[i]] = i;
		mTabIndexMap[visIndex]         = mFilterMap[i];
	}

	layout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding), 1, i);

	setTabOrder();

    connect(mTree->header(), SIGNAL(sectionResized(int, int, int)), this, SLOT(resizeColumn(int, int, int)));
    connect(mTree->header(), SIGNAL(sectionMoved(int, int, int)), this, SLOT(moveColumn(int, int, int)));
    connect(mTree->horizontalScrollBar(), SIGNAL(valueChanged(int)), horizontalScrollBar(), SLOT(setValue(int)));
}

void RecordFilterWidget::resizeColumn(int column, int oldValue, int newValue)
{
    mFilterMap[column]->setFixedWidth(newValue);
}

void RecordFilterWidget::moveColumn(int, int, int)
{
    for (int i = 0; i < mTree->header()->count(); i++) {
		int visIndex = mTree->header()->visualIndex(i);

        layout->removeWidget(mFilterMap[i]);
        if( !mTree->header()->isSectionHidden(i) ) {
            layout->addWidget(mFilterMap[i], 0, visIndex);

            mFilterIndexMap[mFilterMap[i]] = i;
			mTabIndexMap[visIndex]         = mFilterMap[i];
        }
    }

	setTabOrder();
}

void RecordFilterWidget::textFilterChanged()
{
    if( mRowFilterScheduled) return;

    QLineEdit *filter = qobject_cast<QLineEdit*> (sender());
    if( mFilterIndexMap.contains(filter) ) {
        int column = mFilterIndexMap.value(filter);
        SuperModel * sm = (SuperModel *)(mTree->model());
        sm->setColumnFilter( column, filter->text() );
    }
    QTimer::singleShot(500, this, SLOT(filterRows()));
    mRowFilterScheduled = true;
}

void RecordFilterWidget::filterRows()
{
    //QTime t;
    //t.start();
    mRowChildrenVisited.clear();
    filterChildren( mTree->rootIndex() );
    //LOG_3( QString("Took %1 ms to filter rows").arg(t.elapsed()) );
    mRowFilterScheduled = false;
}

/*
void RecordFilterWidget::filterRows()
{
    SuperModel * sm = (SuperModel *)(mTree->model());
    int mapSize = mFilterMap.size();
    QMap<int, QModelIndex> rowsToHide;

    QModelIndexList indexes = ModelIter::collect(sm, ModelIter::Filter(ModelIter::Recursive));
    LOG_3( "View has "+QString::number(indexes.size())+" indexes" );
    foreach( QModelIndex index, indexes ) {
        // reset everything to visible
        mTree->setRowHidden(index.row(), index.parent(), false);

        for ( int col = 0; col < mapSize; col++ ) {

            QLineEdit *filter = qobject_cast<QLineEdit*> (mFilterMap[col]);

            QModelIndex index2 = sm->index(index.row(), mFilterIndexMap[mFilterMap[col]], index.parent());
            QString cell = sm->data(index2).toString();

            if ( filter && filter->isVisible() && !filter->text().isEmpty() ) {
                QString filterText = filter->text();
                LOG_3( "filter for col: "+QString::number(col)+ " is "+filterText+"; row: "+QString::number(index.row())+" value is "+cell );
                if ( !cell.contains(QRegExp(filterText, Qt::CaseInsensitive)) )
                    rowsToHide[ index.row() ] = index.parent();
                sm->setColumnFilter( col, filterText );
            } else
                sm->setColumnFilter( col, "" );
        }

    }

    foreach( int row, rowsToHide.keys() ) {
        mTree->setRowHidden(row, rowsToHide[row], true);
        QString cell = sm->data(rowsToHide[row]).toString();
        //LOG_4("hiding row: "+QString::number(row)+" parent value is: "+cell);
    }
}
*/

int RecordFilterWidget::filterChildren(const QModelIndex & parent)
{
    SuperModel * sm = (SuperModel *)(mTree->model());
    int numRows = sm->rowCount(parent);
    int visibleRows = numRows;
    int mapSize = mFilterMap.size();
    int visibleChildren = 0;
    //LOG_3( "parent has "+QString::number(numRows)+" children" );
    for ( int row = 0; row < numRows; ++row ) {
        mTree->setRowHidden(row, parent, false);

        bool removedRow = false;
        for ( int col = 0; col < mapSize && !removedRow; col++ ) {
            QLineEdit *filter = qobject_cast<QLineEdit*> (mFilterMap[col]);

            if ( filter && filter->isVisible() && !filter->text().isEmpty() ) {
                QModelIndex index = sm->index(row, mFilterIndexMap[mFilterMap[col]], parent);
                QString cell = sm->data(index).toString();
                QString filterText = filter->text();

                visibleChildren = 0;
                if ( !mRowChildrenVisited.contains(index) && sm->hasChildren(index) ) {
                    visibleChildren = filterChildren(index);
                    mRowChildrenVisited[index] = true;
                    //LOG_3( "child "+cell+" has children, filtering - "+QString::number(visibleChildren)+ " of its children are visible" );
                }

                if ( visibleChildren == 0 ) {
                    // faster implementation
                    switch (filterText[0].toAscii())
                    {
                        case '!':
                            filterText = filterText.remove(0,1);
                            if ( cell.contains(QRegExp(filterText, Qt::CaseInsensitive)) ) {
                                mTree->setRowHidden(row, parent, true);
                                visibleRows--;
                                removedRow = true;
                            }
                            break;
                        case '>':
                            filterText = filterText.remove(0,1);
                            if( cell.toInt() <= filterText.toInt() ) {
                                mTree->setRowHidden(row, parent, true);
                                visibleRows--;
                                removedRow = true;
                            }
                            break;
                        case '<':
                            filterText = filterText.remove(0,1);
                            if( cell.toInt() >= filterText.toInt() ) {
                                mTree->setRowHidden(row, parent, true);
                                visibleRows--;
                                removedRow = true;
                            }
                            break;
                        default:
                            if ( !cell.contains(QRegExp(filterText, Qt::CaseInsensitive)) ) {
                                mTree->setRowHidden(row, parent, true);
                                visibleRows--;
                                removedRow = true;
                            }
                    }
                } else
                    break;

                sm->setColumnFilter( col, filterText );
            } else {
                // Set the filter to empty so that the highlighted text gets un-highlighted in recorddelegate
                sm->setColumnFilter( col, "" );
            }

        }
    }

    return visibleRows + visibleChildren;
}

void RecordFilterWidget::clearFilters()
{
	int mapSize = mFilterMap.size();
	for ( int col = 0; col < mapSize; col++ ) {
		QLineEdit *filter = qobject_cast<QLineEdit*> (mFilterMap[col]);

		if ( filter )
			filter->clear();
	}
}

void RecordFilterWidget::setTabOrder()
{
    QLineEdit *prev = NULL;
    QLineEdit *cur  = NULL;
    for (int i = 0; i < mTabIndexMap.size(); i++) {
        if (i > 0) {

            QLineEdit *filter1 = qobject_cast<QLineEdit*> (mTabIndexMap[i-1]);
            QLineEdit *filter2 = qobject_cast<QLineEdit*> (mTabIndexMap[i]);

            if (filter1)
                prev = filter1;
            if (filter2)
                cur = filter2;

            if (prev && cur && prev != cur)
                QWidget::setTabOrder(prev, cur);
        }
    }
}
