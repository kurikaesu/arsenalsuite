/****************************************************************************
**
** Copyright (C) 1992-2006 Trolltech AS. All rights reserved.
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.trolltech.com/products/qt/opensource.html
**
** If you are unsure which license is appropriate for your use, please
** review the following information:
** http://www.trolltech.com/products/qt/licensing.html or contact the
** sales department at sales@trolltech.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QTREEVIEW_P_H
#define QTREEVIEW_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "private/qabstractitemview_p.h"

#ifndef QT_NO_TREEVIEW

struct QTreeViewItem
{
    QTreeViewItem() : expanded(false), total(0), level(0), height(0) {}
    QModelIndex index; // we remove items whenever the indexes are invalidated (make persistent ?)
    uint expanded : 1;
    uint total : 30; // total number of children visible (+ hidden children)
    uint level : 16; // indentation
    int height : 16; // row height
};

class QTreeViewPrivate: public QAbstractItemViewPrivate
{
    Q_DECLARE_PUBLIC(QTreeView)
public:

    QTreeViewPrivate()
        : QAbstractItemViewPrivate(),
          header(0), indent(20), lastViewedItem(0), itemHeight(-1), 
          uniformRowHeights(false), rootDecoration(true),
          itemsExpandable(true), exactScrollbarPositions(true),
          topItemIndex(0), topItemOffset(0),
          columnResizeTimerID(0)  {}

    ~QTreeViewPrivate() {}
    void initialize();

    void expand(int item, bool emitSignal);
    void collapse(int item, bool emitSignal);
    void layout(int item);

    int pageUp(int item) const;
    int pageDown(int item) const;

    inline int above(int item) const
        { return (--item < 0 ? 0 : item); }
    inline int below(int item) const
        { return (++item >= viewItems.count() ? viewItems.count() - 1 : item); }

    inline int height(int item) const {
        if (uniformRowHeights)
            return itemHeight;
        if (viewItems.isEmpty())
            return 0;
        const QModelIndex index = viewItems.at(item).index;
        int height = viewItems.at(item).height;
        if (height <= 0 && index.isValid()) {
            height = q_func()->indexRowSizeHint(index);
            viewItems[item].height = height;
        }
        if (!index.isValid() || height < 0)
            return 0;
        return height;
    }

    inline void invalidateHeightCache(int item) const {
        viewItems[item].height = 0;
    }
        
    int indentation(int item) const;
    int coordinate(int item) const;
    int item(int coordinate) const;

    int viewIndex(const QModelIndex &index) const;
    QModelIndex modelIndex(int i) const;

    int itemAt(int value) const;
    int topItemDelta(int value, int iheight) const;
    int columnAt(int x) const;

    void relayout(const QModelIndex &parent);
    void reexpandChildren(const QModelIndex &parent);

    void updateScrollbars() { updateVerticalScrollbar(); updateHorizontalScrollbar(); }
    void updateVerticalScrollbar();
    void updateHorizontalScrollbar();

    int itemDecorationAt(const QPoint &pos) const;

    void select(int start, int stop, QItemSelectionModel::SelectionFlags command);

    void calculateTopItemIndex();

    QHeaderView *header;
    int indent;

    mutable QVector<QTreeViewItem> viewItems;
    mutable int lastViewedItem;
    int itemHeight; // this is just a number; contentsHeight() / numItems
    bool uniformRowHeights; // used when all rows have the same height
    bool rootDecoration;
    bool itemsExpandable;
    bool exactScrollbarPositions;
    int topItemIndex;
    int topItemOffset;

    // used for drawing
    int left;
    int right;
    int current;

    // used when expanding and closing items
    QVector<QPersistentModelIndex> expandedIndexes;
    QStack<bool> expandParent;

    // used when hiding and showing items
    QVector<QPersistentModelIndex> hiddenIndexes;

    // used for hidden items
    int hiddenItemsCount;

    // used for updating resized columns
    int columnResizeTimerID;
    QList<int> columnsToUpdate;
};

#endif // QT_NO_TREEVIEW

#endif // QTREEVIEW_P_H
