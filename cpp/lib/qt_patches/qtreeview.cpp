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
#include "qtreeview.h"

#ifndef QT_NO_TREEVIEW
#include <qheaderview.h>
#include <qitemdelegate.h>
#include <qapplication.h>
#include <qscrollbar.h>
#include <qpainter.h>
#include <qstack.h>
#include <qstyle.h>
#include <qstyleoption.h>
#include <qevent.h>
#include <qpen.h>
#include <qdebug.h>

#include <private/qtreeview_p.h>

/*!
    \class QTreeView qtreeview.h
    \brief The QTreeView class provides a default model/view implementation of a tree view.

    \ingroup model-view
    \mainclass

    A QTreeView implements a tree representation of items from a
    model. This class is used to provide standard hierarchical lists that
    were previously provided by the \c QListView class, but using the more
    flexible approach provided by Qt's model/view architecture.

    The QTreeView class is one of the \l{Model/View Classes} and is part of
    Qt's \l{Model/View Programming}{model/view framework}.

    QTreeView implements the interfaces defined by the
    QAbstractItemView class to allow it to display data provided by
    models derived from the QAbstractItemModel class.

    It is simple to construct a tree view displaying data from a
    model. In the following example, the contents of a directory are
    supplied by a QDirModel and displayed as a tree:

    \quotefromfile snippets/shareddirmodel/main.cpp
    \skipto QDirModel *model
    \printuntil QTreeView *tree
    \skipto tree->setModel(
    \printuntil tree->setModel(

    The model/view architecture ensures that the contents of the tree view
    are updated as the model changes.

    Items that have children can be in an expanded (children are
    visible) or collapsed (children are hidden) state. When this state
    changes a collapsed() or expanded() signal is emitted with the
    model index of the relevant item.

    The amount of indentation used to indicate levels of hierarchy is
    controlled by the \l indentation property.

    Headers in a tree view are constructed using the QHeaderView class
    and can be hidden using header()->hide().

    \section2 Key Bindings

    QTreeView supports a set of key bindings that enable the user to
    navigate in the view and interact with the contents of items:

    \table
    \header \o Key \o Action
    \row \o UpArrow   \o Moves the cursor to the item in the same column on
         the previous row. If the parent of the current item has no more rows to
         navigate to, the cursor moves to the relevant item in the last row
         of the sibling that precedes the parent.
    \row \o DownArrow \o Moves the cursor to the item in the same column on
         the next row. If the parent of the current item has no more rows to
         navigate to, the cursor moves to the relevant item in the first row
         of the sibling that follows the parent.
    \row \o LeftArrow  \o Hides the children of the current item (if present)
         by collapsing a branch.
    \row \o RightArrow \o Reveals the children of the current item (if present)
         by expanding a branch.
    \row \o PageUp   \o Moves the cursor up one page.
    \row \o PageDown \o Moves the cursor down one page.
    \row \o Home \o Moves the cursor to an item in the same column of the first
         row of the first top-level item in the model.
    \row \o End  \o Moves the cursor to an item in the same column of the last
         row of the last top-level item in the model.
    \row \o F2   \o In editable models, this opens the current item for editing.
         The Escape key can be used to cancel the editing process and revert
         any changes to the data displayed.
    \endtable

    \omit
    Describe the expanding/collapsing concept if not covered elsewhere.
    \endomit

    \table 100%
    \row \o \inlineimage windowsxp-treeview.png Screenshot of a Windows XP style tree view
         \o \inlineimage macintosh-treeview.png Screenshot of a Macintosh style tree view
         \o \inlineimage plastique-treeview.png Screenshot of a Plastique style tree view
    \row \o A \l{Windows XP Style Widget Gallery}{Windows XP style} tree view.
         \o A \l{Macintosh Style Widget Gallery}{Macintosh style} tree view.
         \o A \l{Plastique Style Widget Gallery}{Plastique style} tree view.
    \endtable

    \sa QListView, QTreeWidget, {Model/View Programming}, QAbstractItemModel, QAbstractItemView
*/


/*!
  \fn void QTreeView::expanded(const QModelIndex &index)

  This signal is emitted when the item specified by \a index is expanded.
*/


/*!
  \fn void QTreeView::collapsed(const QModelIndex &index)

  This signal is emitted when the item specified by \a index is collapsed.
*/

/*!
    Constructs a table view with a \a parent to represent a model's
    data. Use setModel() to set the model.

    \sa QAbstractItemModel
*/
QTreeView::QTreeView(QWidget *parent)
    : QAbstractItemView(*new QTreeViewPrivate, parent)
{
    Q_D(QTreeView);
    d->initialize();
}

/*!
  \internal
*/
QTreeView::QTreeView(QTreeViewPrivate &dd, QWidget *parent)
    : QAbstractItemView(dd, parent)
{
    Q_D(QTreeView);
    d->initialize();
}

/*!
  Destroys the tree view.
*/
QTreeView::~QTreeView()
{
}

/*!
  \reimp
*/
void QTreeView::setModel(QAbstractItemModel *model)
{
    Q_D(QTreeView);
    if (d->selectionModel && d->model) { // support row editing
        disconnect(d->selectionModel, SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
                   d->model, SLOT(submit()));
        disconnect(d->model, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
                   this, SLOT(rowsRemoved(const QModelIndex &, int, int)));
    }
    d->topItemOffset = d->topItemIndex = 0;
    d->viewItems.clear();
    d->expandedIndexes.clear();
    d->hiddenIndexes.clear();
    d->header->setModel(model);
    QAbstractItemView::setModel(model);
    if (d->model) {
        // QAbstractItemView connects to a private slot
        disconnect(d->model, SIGNAL(rowsRemoved(QModelIndex, int, int)),
                   this, SLOT(rowsRemoved(QModelIndex, int, int)));
        // QTreeView has a public slot for this
        connect(d->model, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
                this, SLOT(rowsRemoved(const QModelIndex &, int, int)));
    }
}

/*!
  \reimp
*/
void QTreeView::setRootIndex(const QModelIndex &index)
{
    Q_D(QTreeView);
    d->header->setRootIndex(index);
    QAbstractItemView::setRootIndex(index);
}

/*!
  \reimp
*/
void QTreeView::setSelectionModel(QItemSelectionModel *selectionModel)
{
    Q_D(QTreeView);
    Q_ASSERT(selectionModel);
    if (d->model && d->selectionModel) // support row editing
        disconnect(d->selectionModel, SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
                   d->model, SLOT(submit()));

    d->header->setSelectionModel(selectionModel);
    QAbstractItemView::setSelectionModel(selectionModel);

    if (d->model && d->selectionModel) // support row editing
        connect(d->selectionModel, SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
                d->model, SLOT(submit()));
}



/*!
  Returns the header for the tree view.

  \sa QAbstractItemModel::headerData()
*/
QHeaderView *QTreeView::header() const
{
    Q_D(const QTreeView);
    return d->header;
}

/*!
    Sets the header for the tree view, to the given \a header.

    The view takes ownership over the given \a header and deletes it
    when a new header is set.

    \sa QAbstractItemModel::headerData()
*/
void QTreeView::setHeader(QHeaderView *header)
{
    Q_ASSERT(header);
    Q_D(QTreeView);
    if (header == d->header)
        return;
    if (d->header && d->header->parent() == this)
        delete d->header;
    d->header = header;
    d->header->setParent(this);
    if (!d->header->model())
        d->header->setModel(model());

    connect(d->header, SIGNAL(sectionResized(int,int,int)),
            this, SLOT(columnResized(int,int,int)));
    connect(d->header, SIGNAL(sectionMoved(int,int,int)),
            this, SLOT(columnMoved()));
    connect(d->header, SIGNAL(sectionCountChanged(int,int)),
            this, SLOT(columnCountChanged(int,int)));
    connect(d->header, SIGNAL(sectionHandleDoubleClicked(int)),
            this, SLOT(resizeColumnToContents(int)));
    connect(d->header, SIGNAL(sectionClicked(int)),
            this, SLOT(sortByColumn(int)));
    d->header->setFocusProxy(this);
}

/*!
  \property QTreeView::indentation
  \brief indentation of the items in the tree view.

  This property holds the indentation measured in pixels of the items for each
  level in the tree view. For top-level items, the indentation specifies the
  horizontal distance from the viewport edge to the items in the first column;
  for child items, it specifies their indentation from their parent items.
*/
int QTreeView::indentation() const
{
    Q_D(const QTreeView);
    return d->indent;
}

void QTreeView::setIndentation(int i)
{
    Q_D(QTreeView);
    d->indent = i;
}

/*!
  \property QTreeView::rootIsDecorated
  \brief whether to show controls for expanding and collapsing items

  This property holds whether root items are displayed with controls for
  expanding and collapsing them.
*/
bool QTreeView::rootIsDecorated() const
{
    Q_D(const QTreeView);
    return d->rootDecoration;
}

void QTreeView::setRootIsDecorated(bool show)
{
    Q_D(QTreeView);
    d->rootDecoration = show;
    d->viewport->update();
}

/*!
  \property QTreeView::uniformRowHeights
  \brief whether all items in the treeview have the same height

  This property should only be set to true if it is guaranteed that all items
  in the view has the same height. This enables the view to do some
  optimizations.
*/
bool QTreeView::uniformRowHeights() const
{
    Q_D(const QTreeView);
    return d->uniformRowHeights;
}

void QTreeView::setUniformRowHeights(bool uniform)
{
    Q_D(QTreeView);
    d->uniformRowHeights = uniform;
    d->exactScrollbarPositions = !uniform;
    if( d->exactScrollbarPositions )
        d->calculateTopItemIndex();
}

/*!
  \property QTreeView::itemsExpandable
  \brief whether the items are expandable by the user.

  This property holds whether the user can expand and collapse items
  interactively.

*/
bool QTreeView::itemsExpandable() const
{
    Q_D(const QTreeView);
    return d->itemsExpandable;
}

void QTreeView::setItemsExpandable(bool enable)
{
    Q_D(QTreeView);
    d->itemsExpandable = enable;
}

/*!
  Returns the horizontal position of the \a column in the viewport.
*/
int QTreeView::columnViewportPosition(int column) const
{
    Q_D(const QTreeView);
    return d->header->sectionViewportPosition(column);
}

/*!
  Returns the width of the \a column.

  \sa resizeColumnToContents()
*/
int QTreeView::columnWidth(int column) const
{
    Q_D(const QTreeView);
    return d->header->sectionSize(column);
}

/*!
  Returns the column in the tree view whose header covers the \a x
  coordinate given.
*/
int QTreeView::columnAt(int x) const
{
    Q_D(const QTreeView);
    return d->header->logicalIndexAt(x);
}

/*!
    Returns true if the \a column is hidden; otherwise returns false.

    \sa hideColumn(), isRowHidden()
*/
bool QTreeView::isColumnHidden(int column) const
{
    Q_D(const QTreeView);
    return d->header->isSectionHidden(column);
}

/*!
  If \a hide is true the \a column is hidden, otherwise the \a column is shown.

  \sa hideColumn(), setRowHidden()
*/
void QTreeView::setColumnHidden(int column, bool hide)
{
    Q_D(QTreeView);
    if (column < 0 || column >= d->header->count())
        return;
    d->header->setSectionHidden(column, hide);
}

/*!
    Returns true if the item in the given \a row of the \a parent is hidden;
    otherwise returns false.

    \sa setRowHidden(), isColumnHidden()
*/
bool QTreeView::isRowHidden(int row, const QModelIndex &parent) const
{
    Q_D(const QTreeView);
    if (d->hiddenIndexes.isEmpty())
        return false;
    return d->hiddenIndexes.contains(model()->index(row, 0, parent));
}

/*!
  If \a hide is true the \a row with the given \a parent is hidden, otherwise the \a row is shown.

  \sa isRowHidden(), setColumnHidden()
*/
void QTreeView::setRowHidden(int row, const QModelIndex &parent, bool hide)
{
    Q_D(QTreeView);
    if (!model())
        return;
    QModelIndex index = model()->index(row, 0, parent);
    if (!index.isValid())
        return;

    QPersistentModelIndex persistent(index);
    if (hide) {
        if (!d->hiddenIndexes.contains(persistent)) d->hiddenIndexes.append(persistent);
    } else {
        int i = d->hiddenIndexes.indexOf(persistent);
        if (i >= 0) d->hiddenIndexes.remove(i);
    }

    if (isVisible())
        d->relayout(parent);
    else
        d->doDelayedItemsLayout();

}

/*!
  \reimp
*/
void QTreeView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    Q_D(QTreeView);

    // if we are going to do a complete realyout anyway, there is no need to update
    if (!model() || d->delayedLayout.isActive())
        return;

    // refresh the height cache here; we don't really lose anything by getting the size hint,
    // since QAbstractItemView::dataChanged() will get the visualRect for the items anyway

    QModelIndex top = (topLeft.column() == 0) ? topLeft
                      : model()->sibling(topLeft.row(), 0, topLeft);
    int topViewIndex = d->viewIndex(top);
    bool sizeChanged = false;
    int delta = 0;
    if (topViewIndex != -1) {
        if (topLeft == bottomRight) {
            int oldHeight = d->height(topViewIndex);
            d->invalidateHeightCache(topViewIndex);
            sizeChanged = (oldHeight != d->height(topViewIndex));
            if( d->exactScrollbarPositions )
                delta = d->height(topViewIndex) - oldHeight;
        } else {
            QModelIndex bottom = (bottomRight.column() == 0) ? bottomRight
                                 : model()->sibling(bottomRight.row(), 0, bottomRight);
            int bottomViewIndex = d->viewIndex(bottom);
            for (int i = topViewIndex; i <= bottomViewIndex; ++i) {
                int oldHeight = d->height(i);
                d->invalidateHeightCache(i);
                sizeChanged |= (oldHeight != d->height(i));
                if( d->exactScrollbarPositions )
                    delta += d->height(i) - oldHeight;
            }
        }
    }

    if( d->exactScrollbarPositions ) {
        verticalScrollBar()->setRange(0,verticalScrollBar()->maximum()+delta);
        d->calculateTopItemIndex();
    }

    if (sizeChanged)
        d->viewport->update();
    QAbstractItemView::dataChanged(topLeft, bottomRight);
}

/*!
  Hides the \a column given.

  \sa showColumn(), setColumnHidden()
*/
void QTreeView::hideColumn(int column)
{
    Q_D(QTreeView);
    d->header->hideSection(column);
}

/*!
  Shows the given \a column in the tree view.

  \sa hideColumn(), setColumnHidden()
*/
void QTreeView::showColumn(int column)
{
    Q_D(QTreeView);
    d->header->showSection(column);
}

/*!
  \fn void QTreeView::expand(const QModelIndex &index)

  Expands the model item specified by the \a index.

  \sa expanded()
*/
void QTreeView::expand(const QModelIndex &index)
{
    Q_D(QTreeView);
    if (!index.isValid())
        return;
    int i = d->viewIndex(index);
    if (i != -1) { // is visible
        d->expand(i, true);
        updateGeometries();
        viewport()->update();
    } else {
        d->expandedIndexes.append(index);
        emit expanded(index);
    }
}

/*!
  \fn void QTreeView::collapse(const QModelIndex &index)

  Collapses the model item specified by the \a index.

  \sa collapsed()
*/
void QTreeView::collapse(const QModelIndex &index)
{
    Q_D(QTreeView);
    if (!index.isValid())
        return;
    int i = d->viewIndex(index);
    if (i != -1) { // is visible
        d->collapse(i, true);
        updateGeometries();
        viewport()->update();
    } else {
        int i = d->expandedIndexes.indexOf(index);
        if (i != -1) {
            d->expandedIndexes.remove(i);
            emit collapsed(index);
        }
    }
}

/*!
  \fn bool QTreeView::isExpanded(const QModelIndex &index) const

  Returns true if the model item \a index is expanded; otherwise returns
  false.

  \da expand(), expanded()
*/
bool QTreeView::isExpanded(const QModelIndex &index) const
{
    Q_D(const QTreeView);
    int i = d->viewIndex(index);
    if (i != -1) // is visible
        return d->viewItems.at(i).expanded;
    return d->expandedIndexes.contains(index);
}

/*!
  Sets the item referred to by \a index to either collapse or expanded,
  depending on the value of \a expanded.

  \sa expanded(), expand()
*/
void QTreeView::setExpanded(const QModelIndex &index, bool expanded)
{
    if (expanded)
        this->expand(index);
    else
        this->collapse(index);
}

/*!
  \reimp
 */
void QTreeView::keyboardSearch(const QString &search)
{
    Q_D(QTreeView);
    if (!model() || !model()->rowCount(rootIndex()) || !model()->columnCount(rootIndex()))
        return;

    QModelIndex start;
    if (currentIndex().isValid())
        start = currentIndex();
    else
        start = model()->index(0, 0, rootIndex());

    QTime now(QTime::currentTime());
    bool skipRow = false;
    if (d->keyboardInputTime.msecsTo(now) > QApplication::keyboardInputInterval()) {
        d->keyboardInput = search;
        skipRow = true;
    } else {
        d->keyboardInput += search;
    }
    d->keyboardInputTime = now;

    // special case for searches with same key like 'aaaaa'
    bool sameKey = false;
    if (d->keyboardInput.length() > 1) {
        int c = d->keyboardInput.count(d->keyboardInput.at(d->keyboardInput.length() - 1));
        sameKey = (c == d->keyboardInput.length());
        if (sameKey)
            skipRow = true;
    }

    // skip if we are searching for the same key or a new search started
    if (skipRow) {
        if (indexBelow(start).isValid())
            start = indexBelow(start);
        else
            start = model()->index(0, start.column(), rootIndex());
    }

    int startIndex = d->viewIndex(start);
    if (startIndex <= -1)
        return;

    int previousLevel = -1;
    int bestAbove = -1;
    int bestBelow = -1;
    QString searchString = sameKey ? QString(d->keyboardInput.at(0)) : d->keyboardInput;
    for (int i = 0; i < d->viewItems.count(); ++i) {
        if ((int)d->viewItems.at(i).level > previousLevel) {
            QModelIndex searchFrom = d->viewItems.at(i).index;
            if (searchFrom.parent() == start.parent())
                searchFrom = start;
            QModelIndexList match = model()->match(searchFrom, Qt::DisplayRole, searchString);
            if (match.count()) {
                int hitIndex = d->viewIndex(match.at(0));
                if (hitIndex >= 0 && hitIndex < startIndex)
                    bestAbove = bestAbove == -1 ? hitIndex : qMin(hitIndex, bestAbove);
                else if (hitIndex >= startIndex)
                    bestBelow = bestBelow == -1 ? hitIndex : qMin(hitIndex, bestBelow);
            }
        }
        previousLevel = d->viewItems.at(i).level;
    }

    QModelIndex index;
    if (bestBelow > -1)
        index = d->viewItems.at(bestBelow).index;
    else if (bestAbove > -1)
        index = d->viewItems.at(bestAbove).index;

    if (index.isValid()) {
        QItemSelectionModel::SelectionFlags flags = (d->selectionMode == SingleSelection
                                                     ? QItemSelectionModel::SelectionFlags(
                                                         QItemSelectionModel::ClearAndSelect
                                                         |d->selectionBehaviorFlags())
                                                     : QItemSelectionModel::SelectionFlags(
                                                         QItemSelectionModel::NoUpdate));
        selectionModel()->setCurrentIndex(index, flags);
    }
}

/*!
  Returns the rectangle on the viewport occupied by the item at \a index.
  If the index is not visible or explicitly hidden, the returned rectangle is invalid.
*/
QRect QTreeView::visualRect(const QModelIndex &index) const
{
    Q_D(const QTreeView);

    if (!index.isValid() || isIndexHidden(index))
        return QRect();

    d->executePostedLayout();

    int vi = d->viewIndex(index);
    if (vi < 0)
        return QRect();

    int x = columnViewportPosition(index.column());
    int w = columnWidth(index.column());

    if (index.column() == 0) {
        int i = d->indentation(vi);
        x += i;
        w -= i;
    }
    int y = d->coordinate(vi);
    int h = d->height(vi);
    return QRect(x, y, w, h);
}

/*!
    Scroll the contents of the tree view until the given model item
    \a index is visible. The \a hint parameter specifies more
    precisely where the item should be located after the
    operation.
    If any of the parents of the model item are collapsed, they will
    be expanded to ensure that the model item is visible.
*/
void QTreeView::scrollTo(const QModelIndex &index, ScrollHint hint)
{
    Q_D(QTreeView);

    if (!model() || !index.isValid())
        return;

    // Expand all parents if the parent(s) of the node are not expanded.
    QModelIndex parent = index.parent();
    while (parent.isValid() && state() != CollapsingState && d->itemsExpandable) {
        if (!isExpanded(parent))
            expand(parent);
        parent = model()->parent(parent);
    }

    QRect rect = visualRect(index);
    if (rect.isEmpty())
        return;

    // check if we really need to do anything
    QRect area = d->viewport->rect();
    if (hint == EnsureVisible && area.contains(rect)) {
        d->setDirtyRegion(rect);
        return;
    }

    bool above = (hint == EnsureVisible && rect.top() < area.top());
    bool below = (hint == EnsureVisible && rect.bottom() > area.bottom());

    // Account for large rows
    if( !above && below && rect.height() > area.height() ) {
        below = false;
        above = true;
    }

    if( d->exactScrollbarPositions ) {
        int item = d->viewIndex(index);
        int pos = d->coordinate(item);
        int cv = verticalScrollBar()->value();
        int vscroll = 0;
        if( hint == PositionAtTop || above )
            vscroll = pos;
        else if( hint == PositionAtBottom || below )
            vscroll = pos - area.height() + d->height(item);
        verticalScrollBar()->setValue( cv + vscroll );
        rect.translate( 0, -vscroll );
    } else {
        // vertical
        int verticalSteps = verticalStepsPerItem();
        if (hint == PositionAtTop || above) {
            int i = d->viewIndex(index);
            verticalScrollBar()->setValue(i * verticalSteps);
        } else if (hint == PositionAtBottom || below) {
            int i = d->viewIndex(index);
            if (i < 0) {
                qWarning("scrollTo: item index was illegal: %d", i);
                return;
            }
            int y = area.height();
            while (y > 0 && i > 0)
                y -= d->height(i--);
            int h = d->height(i);
            int a = (-y * verticalSteps) / (h ? h : 1);
            verticalScrollBar()->setValue(++i * verticalSteps + a);
        }
    }

    // horizontal
    int viewportWidth = d->viewport->width();
    int horizontalOffset = d->header->offset();
    int horizontalPosition = d->header->sectionPosition(index.column());
    int cellWidth = d->header->sectionSize(index.column());

    if (horizontalPosition - horizontalOffset < 0 || cellWidth > viewportWidth)
        horizontalScrollBar()->setValue(horizontalPosition);
    else if (horizontalPosition - horizontalOffset + cellWidth > viewportWidth)
        horizontalScrollBar()->setValue(horizontalPosition - viewportWidth + cellWidth);
}

/*!
  \reimp
*/
void QTreeView::timerEvent(QTimerEvent *event)
{
    Q_D(QTreeView);
    if (event->timerId() == d->columnResizeTimerID) {
        updateGeometries();
        killTimer(d->columnResizeTimerID);
        d->columnResizeTimerID = 0;
        QRect rect;
        int viewportHeight = d->viewport->height();
        int viewportWidth = d->viewport->width();
        for (int i = d->columnsToUpdate.size() - 1; i >= 0; --i) {
            int column = d->columnsToUpdate.at(i);
            int x = columnViewportPosition(column);
            if (isRightToLeft())
                rect |= QRect(0, 0, x + columnWidth(column), viewportHeight);
            else
                rect |= QRect(x, 0, viewportWidth - x, viewportHeight);
        }
        d->viewport->update(rect.normalized());
        d->columnsToUpdate.clear();
    }
    QAbstractItemView::timerEvent(event);
}

/*!
  \reimp
*/
void QTreeView::paintEvent(QPaintEvent *event)
{
    Q_D(QTreeView);
    QStyleOptionViewItem option = viewOptions();
    const QStyle::State state = option.state;
    const int v = verticalScrollBar()->value();
    const int c = d->viewItems.count();
    const QVector<QTreeViewItem> viewItems = d->viewItems;
    const QPoint offset = d->scrollDelayOffset;

    QPainter painter(d->viewport);
    if (c == 0 || d->header->count() == 0) {
        painter.fillRect(event->rect(), option.palette.brush(QPalette::Base));
        return;
    }

    QVector<QRect> rects = event->region().rects();
    for (int a = 0; a < rects.size(); ++a) {

        QRect area = rects.at(a);
        area.translate(offset);

        const int t = area.top();
        const int b = area.bottom() + 1;

        d->left = d->header->visualIndexAt(area.left());
        d->right = d->header->visualIndexAt(area.right());
        if (isRightToLeft()) {
            d->left = (d->left == -1 ? d->header->count() - 1 : d->left);
            d->right = (d->right == -1 ? 0 : d->right);
        } else {
            d->left = (d->left == -1 ? 0 : d->left);
            d->right = (d->right == -1 ? d->header->count() - 1 : d->right);
        }

        int tmp = d->left;
        d->left = qMin(d->left, d->right);
        d->right = qMax(tmp, d->right);

        int i = d->itemAt(v); // first item
        if (i < 0) // couldn't find the first item
            return;
        int y = d->topItemDelta(v, d->height(i));
        int w = d->viewport->width();

        while (y < b && i < c) {
            int h = d->height(i); // actual height
            if (y + h >= t) { // we are in the update area
                option.rect.setRect(0, y, 0, h);
                option.state = state | (viewItems.at(i).expanded
                                        ? QStyle::State_Open : QStyle::State_None);
                d->current = i;
                drawRow(&painter, option, viewItems.at(i).index);
            }
            y += h;
            ++i;
        }

        int x = d->header->length();
        QRect bottom(0, y, w, b - y);
        if (y < b && area.intersects(bottom))
            painter.fillRect(bottom, option.palette.brush(QPalette::Base));
        if (isRightToLeft()) {
            QRect right(0, 0, w - x, b);
            if (x < w && area.intersects(right))
                painter.fillRect(right, option.palette.brush(QPalette::Base));
        } else {
            QRect left(x, 0, w - x, b);
            if (x < w && area.intersects(left))
                painter.fillRect(left, option.palette.brush(QPalette::Base));
        }
    }

#ifndef QT_NO_DRAGANDDROP
    // Paint the dropIndicator
    d_func()->paintDropIndicator(&painter);
#endif
}

/*!
  Draws the row in the tree view that contains the model item \a index,
  using the \a painter given. The \a option control how the item is
  displayed.

  \sa QStyleOptionViewItem(), setAlternatingRowColors()
*/
void QTreeView::drawRow(QPainter *painter, const QStyleOptionViewItem &option,
                        const QModelIndex &index) const
{
    Q_D(const QTreeView);
    const QPoint offset = d->scrollDelayOffset;
    QStyleOptionViewItem opt = option;
    const int y = option.rect.y() + offset.y();
    const QModelIndex parent = index.parent();
    const QHeaderView *header = d->header;
    const QModelIndex current = currentIndex();
    const QModelIndex hover = d->hover;
    const bool focus = (hasFocus() || d->viewport->hasFocus()) && current.isValid();
    const bool reverse = isRightToLeft();
    const QStyle::State state = opt.state;
    const int left = d->left;
    const int right = d->right;
    const bool alternate = d->alternatingColors;
    const bool enabled = (state & QStyle::State_Enabled) != 0;

    // ### special case: treeviews with multiple columns draw the selections differently than with only one column
    opt.showDecorationSelected = (d->selectionBehavior & SelectRows)
                                 || option.showDecorationSelected;

    int width, height = option.rect.height();
    int position;
    int headerSection;
    QModelIndex modelIndex;

    for (int headerIndex = left; headerIndex <= right; ++headerIndex) {
        headerSection = d->header->logicalIndex(headerIndex);
        if (header->isSectionHidden(headerSection))
            continue;
        position = columnViewportPosition(headerSection) + offset.x();
        width = header->sectionSize(headerSection);
        modelIndex = d->model->index(index.row(), headerSection, parent);
        opt.state = state;
        if (!modelIndex.isValid()) {
            opt.rect.setRect(position, y, width, height);
            painter->fillRect(opt.rect, opt.palette.brush(QPalette::Base));
            continue;
        }
        if (selectionModel()->isSelected(modelIndex))
            opt.state |= QStyle::State_Selected;
        if (focus && current == modelIndex)
            opt.state |= QStyle::State_HasFocus;
        if (modelIndex == hover)
            opt.state |= QStyle::State_MouseOver;
        else
            opt.state &= ~QStyle::State_MouseOver;
        if (enabled) {
            QPalette::ColorGroup cg;
            if ((model()->flags(index) & Qt::ItemIsEnabled) == 0) {
                opt.state &= ~QStyle::State_Enabled;
                cg = QPalette::Disabled;
            } else {
                cg = QPalette::Normal;
            }
            opt.palette.setCurrentColorGroup(cg);
        }
        QBrush fill;
        if (alternate) {
            fill = d->current & 1
                    ? opt.palette.brush(QPalette::AlternateBase)
                    : opt.palette.brush(QPalette::Base);
        } else {
            fill = opt.palette.brush(QPalette::Base);
        }
        if (headerSection == 0) {
            int i = d->indentation(d->current);
            opt.rect.setRect(reverse ? position : i + position, y, width - i, height);
            painter->fillRect(opt.rect, fill);
            QRect branches(reverse ? position + width - i : position, y, i, height);
            if ((opt.state & QStyle::State_Selected) && option.showDecorationSelected)
                painter->fillRect(branches, option.palette.brush(QPalette::Highlight));
            else
                painter->fillRect(branches, fill);
            drawBranches(painter, branches, index);
        } else {
            opt.rect.setRect(position, y, width, height);
            painter->fillRect(opt.rect, fill);
        }
        itemDelegate()->paint(painter, opt, modelIndex);
    }
}

/*!
  Draws the branches in the tree view on the same row as the model item
  \a index, using the \a painter given. The branches are drawn in the
  rectangle specified by \a rect.
*/
void QTreeView::drawBranches(QPainter *painter, const QRect &rect,
                             const QModelIndex &index) const
{
    Q_D(const QTreeView);
    const bool reverse = isRightToLeft();
    const int indent = d->indent;
    const int outer = d->rootDecoration ? 0 : 1;
    const int item = d->current;
    int level = d->viewItems.at(item).level;
    QRect primitive(reverse ? rect.left() : rect.right(), rect.top(), indent, rect.height());


    const QModelIndex parent = index.parent();
    QModelIndex current = parent;
    QModelIndex ancestor = current.parent();

    QStyleOption opt;
    opt.initFrom(this);
    QStyle::State extraFlags = QStyle::State_None;
    if (isEnabled())
        extraFlags |= QStyle::State_Enabled;
    if (window()->isActiveWindow())
        extraFlags |= QStyle::State_Active;

    QPoint oldBO = painter->brushOrigin();

    int v = verticalScrollBar()->value();
    int i = d->itemAt(v);
    if (i >= 0) {
      int delta = d->topItemDelta(v, d->height(i));
      int offset = verticalOffset();

      painter->setBrushOrigin(QPoint(0, -1 * offset - delta));
    }

    if (level >= outer) {
        // start with the innermost branch
        primitive.moveLeft(reverse ? primitive.left() : primitive.left() - indent);
        opt.rect = primitive;

        const bool expanded = d->viewItems.at(item).expanded;
        const bool children = (expanded // already layed out
                               ? d->viewItems.at(item).total // this also covers the hidden items
                               : d->model->hasChildren(index)); // not layed out yet, so we don't know
        bool moreSiblings = false;
        if (d->hiddenIndexes.isEmpty())
            moreSiblings = (d->model->rowCount(parent) - 1 > index.row());
        else
            moreSiblings = ((d->viewItems.size() > item +1) && (d->viewItems.at(item + 1).index.parent() == parent));
        opt.state = QStyle::State_Item | extraFlags
                    | (moreSiblings ? QStyle::State_Sibling : QStyle::State_None)
                    | (children ? QStyle::State_Children : QStyle::State_None)
                    | (expanded ? QStyle::State_Open : QStyle::State_None);
        style()->drawPrimitive(QStyle::PE_IndicatorBranch, &opt, painter, this);
    }
    // then go out level by level
    for (--level; level >= outer; --level) { // we have already drawn the innermost branch
        primitive.moveLeft(reverse ? primitive.left() + indent : primitive.left() - indent);
        opt.rect = primitive;
        opt.state = extraFlags;
        if (d->model->rowCount(ancestor) - 1 > current.row())
            opt.state |= QStyle::State_Sibling;
        style()->drawPrimitive(QStyle::PE_IndicatorBranch, &opt, painter, this);
        current = ancestor;
        ancestor = current.parent();
    }
    painter->setBrushOrigin(oldBO);
}

/*!
  \reimp
*/
void QTreeView::mousePressEvent(QMouseEvent *event)
{
    Q_D(QTreeView);
    if (!d->viewport->rect().contains(event->pos()))
        return;
    int i = d->itemDecorationAt(event->pos());
    if (i == -1) {
        QAbstractItemView::mousePressEvent(event);
    } else if (itemsExpandable() && model()->hasChildren(d->viewItems.at(i).index)) {
        if (d->viewItems.at(i).expanded)
            d->collapse(i, true);
        else
            d->expand(i, true);
        updateGeometries();
        viewport()->update();
    }
}

/*!
  \reimp
*/
void QTreeView::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QTreeView);
    if (d->itemDecorationAt(event->pos()) == -1)
        QAbstractItemView::mouseReleaseEvent(event);
}

/*!
  \reimp
*/
void QTreeView::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_D(QTreeView);
    if (!d->viewport->rect().contains(event->pos()))
        return;

    int i = d->itemDecorationAt(event->pos());
    if (i == -1) {
        i = d->item(event->y());
        if (i == -1)
            return; // user clicked outside the items

        // signal handlers may change the model
        QModelIndex index = d->viewItems.at(i).index;
        int column = d->header->logicalIndexAt(event->x());
        QPersistentModelIndex persistent = index.sibling(index.row(), column);
        emit doubleClicked(persistent);

        if (edit(persistent, DoubleClicked, event) || state() != NoState)
            return; // the double click triggered editing

        if (!style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick))
            emit activated(persistent);

        d->executePostedLayout(); // we need to make sure viewItems is updated
        if (d->itemsExpandable && model()->hasChildren(d->viewItems.at(i).index)) {
            if (d->viewItems.at(i).expanded)
                d->collapse(i, true);
            else
                d->expand(i, true);
            updateGeometries();
            viewport()->update();
        }
    }
}

/*!
  \reimp
*/
QModelIndex QTreeView::indexAt(const QPoint &point) const
{
    Q_D(const QTreeView);
    d->executePostedLayout();

    int visualIndex = d->item(point.y());
    QModelIndex idx = d->modelIndex(visualIndex);
    int column = d->columnAt(point.x());
    if (idx.isValid() && column >= 0)
        return model()->sibling(idx.row(), column, idx);
    return QModelIndex();
}

/*!
  Returns the model index of the item above \a index.
*/
QModelIndex QTreeView::indexAbove(const QModelIndex &index) const
{
    Q_D(const QTreeView);
    if (!index.isValid())
        return QModelIndex();
    d->executePostedLayout();
    int i = d->viewIndex(index);
    if (--i < 0)
        return QModelIndex();
    return d->viewItems.at(i).index;
}

/*!
  Returns the model index of the item below \a index.
*/
QModelIndex QTreeView::indexBelow(const QModelIndex &index) const
{
    Q_D(const QTreeView);
    d->executePostedLayout();
    int i = d->viewIndex(index);
    if (++i >= d->viewItems.count())
        return QModelIndex();
    return d->viewItems.at(i).index;
}

/*!
  Lays out the items in the tree view.
*/
void QTreeView::doItemsLayout()
{
    Q_D(QTreeView);
    d->viewItems.clear(); // prepare for new layout
    QStyleOptionViewItem option = viewOptions();
    QModelIndex parent = rootIndex();
    if (model() && model()->hasChildren(parent)) {
        QModelIndex index = model()->index(0, 0, parent);
        d->itemHeight = indexRowSizeHint(index);
        d->layout(-1);
        d->reexpandChildren(parent);
        d->calculateTopItemIndex();
    }
    QAbstractItemView::doItemsLayout();
}

/*!
  \reimp
*/
void QTreeView::reset()
{
    Q_D(QTreeView);
    d->expandedIndexes.clear();
    d->hiddenIndexes.clear();
    d->viewItems.clear();
    d->topItemOffset = d->topItemIndex = 0;
    QAbstractItemView::reset();
}

/*!
  Returns the horizontal offset of the items in the treeview.

  Note that the tree view uses the horizontal header section
  positions to determine the positions of columns in the view.

  \sa verticalOffset()
*/
int QTreeView::horizontalOffset() const
{
    Q_D(const QTreeView);
    return d->header->offset();
}

/*!
  Returns the vertical offset of the items in the tree view.

  \sa horizontalOffset()
*/
int QTreeView::verticalOffset() const
{
    Q_D(const QTreeView);
    if( d->exactScrollbarPositions )
        return verticalScrollBar()->value();

    // gives an estimate
    if (model() && model()->rowCount(rootIndex()) > 0) {
        float items = float(verticalScrollBar()->value()) / float(verticalStepsPerItem());
        return int(items * d->itemHeight);
    }
    // no items, no offset
    return 0;
}

/*!
    Move the cursor in the way described by \a cursorAction, using the
    information provided by the button \a modifiers.
*/
QModelIndex QTreeView::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    Q_D(QTreeView);
    Q_UNUSED(modifiers);

    QModelIndex current = currentIndex();
    if (!current.isValid()) {
        int i = 0;
        while (i < d->viewItems.count() && d->hiddenIndexes.contains(d->viewItems.at(i).index))
            ++i;
        return d->viewItems.value(i).index;
    }
    int vi = qMax(0, d->viewIndex(current));
    switch (cursorAction) {
    case MoveNext:
    case MoveDown:
#ifdef QT_KEYPAD_NAVIGATION
        if (vi == d->viewItems.count()-1 && QApplication::keypadNavigationEnabled())
            return model()->index(0, 0, rootIndex());
#endif
        return d->modelIndex(d->below(vi));
    case MovePrevious:
    case MoveUp:
#ifdef QT_KEYPAD_NAVIGATION
        if (vi == 0 && QApplication::keypadNavigationEnabled())
            return d->modelIndex(d->viewItems.count() - 1);
#endif
        return d->modelIndex(d->above(vi));
    case MoveLeft:
        if (d->viewItems.at(vi).expanded && d->itemsExpandable)
            d->collapse(vi, true);
        updateGeometries();
        viewport()->update();
        break;
    case MoveRight:
        if (!d->viewItems.at(vi).expanded && d->itemsExpandable)
            d->expand(vi, true);
        updateGeometries();
        viewport()->update();
        break;
    case MovePageUp:
        return d->modelIndex(d->pageUp(vi));
    case MovePageDown:
        return d->modelIndex(d->pageDown(vi));
    case MoveHome:
        return model()->index(0, 0, rootIndex());
    case MoveEnd:
        return d->modelIndex(d->viewItems.count() - 1);
    }
    return current;
}

/*!
  Applies the selection \a command to the items in or touched by the
  rectangle, \a rect.

  \sa selectionCommand()
*/
void QTreeView::setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command)
{
    Q_D(QTreeView);
    if (!selectionModel())
        return;

    QPoint tl(isRightToLeft() ? rect.right() : rect.left(), rect.top());
    QPoint br(isRightToLeft() ? rect.left() : rect.right(), rect.bottom());
    QModelIndex topLeft = indexAt(tl);
    QModelIndex bottomRight = indexAt(br);
    if (selectionBehavior() != SelectRows) {
        QItemSelection selection;
        if (topLeft.isValid() && bottomRight.isValid()) {
            selection.append(QItemSelectionRange(topLeft, bottomRight));
            selectionModel()->select(selection, command);
        }
    } else {
        d->select(d->viewIndex(topLeft), d->viewIndex(bottomRight), command);
    }
}

/*!
  Returns the rectangle from the viewport of the items in the given
  \a selection.
*/
QRegion QTreeView::visualRegionForSelection(const QItemSelection &selection) const
{
    if (selection.isEmpty())
        return QRegion();

    QRegion selectionRegion;
    for (int i = 0; i < selection.count(); ++i) {
        QItemSelectionRange range = selection.at(i);
        if (!range.isValid())
            continue;
        QModelIndex parent = range.parent();
        QModelIndex leftIndex = range.topLeft();
        int columnCount = model()->columnCount(parent);
        while (leftIndex.isValid() && isIndexHidden(leftIndex)) {
            if (leftIndex.column() + 1 < columnCount)
                leftIndex = model()->index(leftIndex.row(), leftIndex.column() + 1, parent);
            else
                leftIndex = QModelIndex();
        }
        if (!leftIndex.isValid())
            continue;
        int top = visualRect(leftIndex).top();
        QModelIndex rightIndex = range.bottomRight();
        while (rightIndex.isValid() && isIndexHidden(rightIndex)) {
            if (leftIndex.column() - 1 >= 0)
                rightIndex = model()->index(rightIndex.row(), rightIndex.column() - 1, parent);
            else
                rightIndex = QModelIndex();
        }
        if (!rightIndex.isValid())
            continue;
        int bottom = visualRect(rightIndex).bottom();
        if (top > bottom)
            qSwap<int>(top, bottom);
        int height = bottom - top + 1;
        for (int c = range.left(); c <= range.right(); ++c)
            selectionRegion += QRegion(QRect(columnViewportPosition(c), top,
                                             columnWidth(c), height));
    }
    return selectionRegion;
}

/*!
  \reimp
*/
QModelIndexList QTreeView::selectedIndexes() const
{
    QModelIndexList viewSelected;
    QModelIndexList modelSelected;
    if (selectionModel())
        modelSelected = selectionModel()->selectedIndexes();
    for (int i = 0; i < modelSelected.count(); ++i) {
        // check that neither the parents nor the index is hidden before we add
        QModelIndex index = modelSelected.at(i);
        while (index.isValid() && !isIndexHidden(index))
            index = index.parent();
        if (index.isValid())
            continue;
        viewSelected.append(modelSelected.at(i));
    }
    return viewSelected;
}

/*!
  Scrolls the contents of the tree view by (\a dx, \a dy).
*/
void QTreeView::scrollContentsBy(int dx, int dy)
{
    Q_D(QTreeView);

    dx = isRightToLeft() ? -dx : dx;

    if (dx)
        d->header->setOffset(horizontalScrollBar()->value());

    if (d->viewItems.isEmpty())
        return;

    if( d->exactScrollbarPositions ) {
        int v = verticalScrollBar()->value();
        d->topItemOffset -= dy;
        while( d->topItemOffset < 0 && d->topItemIndex > 0 ) {
            d->topItemOffset += d->height(d->topItemIndex-1);
            d->topItemIndex--;
        }
        while( d->topItemOffset >= d->height(d->topItemIndex) && d->topItemIndex < d->viewItems.size() - 1 ) {
            d->topItemOffset -= d->height(d->topItemIndex);
            d->topItemIndex++;
        }
/*        printf( "Adjusted topItemIndex=%i, topItemOffset=%i\n", d->topItemIndex, d->topItemOffset );
        d->topItemIndex = 0;
        d->topItemOffset = 0;
        int tii = d->itemAt(v);
        int itemPos = d->coordinate(tii);
        d->topItemIndex = tii;
        d->topItemOffset = -itemPos;
        printf( "Re-calculated topItemIndex=%i, topItemOffset=%i\n", d->topItemIndex, d->topItemOffset );  */
    } else {
        // guestimate the number of items in the viewport
        int viewCount = d->viewport->height() / d->itemHeight;
        int maxDeltaY = verticalStepsPerItem() * qMin(d->viewItems.count(), viewCount);

        // no need to do a lot of work if we are going to redraw the whole thing anyway
        if (qAbs(dy) > qAbs(maxDeltaY) && d->editors.isEmpty()) {
            verticalScrollBar()->repaint();
            d->viewport->update();
            return;
        }

        if (dy) {
            int steps = verticalStepsPerItem();
            int currentScrollbarValue = verticalScrollBar()->value();
            int previousScrollbarValue = currentScrollbarValue + dy; // -(-dy)
            int currentViewIndex = currentScrollbarValue / steps; // the first visible item
            int previousViewIndex = previousScrollbarValue / steps;

            const QVector<QTreeViewItem> viewItems = d->viewItems;

            int currentY = d->topItemDelta(currentScrollbarValue, d->height(currentViewIndex));
            int previousY = currentY;
            if ((previousViewIndex >= 0) && (previousViewIndex < d->viewItems.size()))
                previousY = d->topItemDelta(previousScrollbarValue, d->height(previousViewIndex));

            dy = currentY - previousY;
            if (previousViewIndex < currentViewIndex) { // scrolling down
                for (int i = previousViewIndex; i < currentViewIndex; ++i)
                    dy -= d->height(i);
            } else if (previousViewIndex > currentViewIndex) { // scrolling up
                for (int i = previousViewIndex - 1; i >= currentViewIndex; --i)
                    dy += d->height(i);
            }
        }
    }

    d->scrollContentsBy(dx, dy);
}

/*!
  This slot is called whenever a column has been moved.
*/
void QTreeView::columnMoved()
{
    QAbstractItemView::dataChanged(QModelIndex(), QModelIndex());
}

/*!
  \internal
*/
void QTreeView::reexpand()
{
    // do nothing
}

/*!
  Informs the view that the rows from the \a start row to the \a end row
  inclusive have been inserted into the \a parent model item.
*/
void QTreeView::rowsInserted(const QModelIndex &parent, int start, int end)
{
    Q_D(QTreeView);
    d->doDelayedItemsLayout();
    QAbstractItemView::rowsInserted(parent, start, end);
}

/*!
  Informs the view that the rows from the \a start row to the \a end row
  inclusive are about to removed from the given \a parent model item.
*/
void QTreeView::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    Q_D(QTreeView);
    if (d->viewItems.isEmpty())
        return;

    setState(CollapsingState);

    if (parent == rootIndex()) {
        d->viewItems.clear();
        d->doDelayedItemsLayout();
        return;
    }

    // collapse the parent
    bool expanded = isExpanded(parent);
    d->expandParent.push(expanded);
    if (expanded) {
        int p = d->viewIndex(parent);
        if (p != -1)
            d->collapse(p, false);
    }

    QAbstractItemView::rowsAboutToBeRemoved(parent, start, end);
}

/*!
    \since 4.1

    Informs the view that the rows from the \a start row to the \a end row
    inclusive have been removed from the given \a parent model item.
*/
void QTreeView::rowsRemoved(const QModelIndex &parent, int start, int end)
{
    Q_UNUSED(start);
    Q_UNUSED(end);
    Q_D(QTreeView);

    if (d->viewItems.isEmpty())
        return;

    if (parent == rootIndex()) {
        d->viewItems.clear();
        d->doDelayedItemsLayout();
        return;
    }

    bool expanded = d->expandParent.pop();
    if (expanded) {
        int p = d->viewIndex(parent);
        if (p != -1) { // item is visible
            d->expand(p, false);
            d->viewport->update();
        } else if (!d->expandedIndexes.contains(parent)) {
            d->expandedIndexes.append(parent);
        }
    }

    setState(NoState);
}

/*!
  Informs the tree view that the number of columns in the tree view has
  changed from \a oldCount to \a newCount.
*/
void QTreeView::columnCountChanged(int, int)
{
    if (isVisible())
        updateGeometries();
	viewport()->update();
}

/*!
  Resizes the \a column given to the size of its contents.
*/
void QTreeView::resizeColumnToContents(int column)
{
    Q_D(QTreeView);
    d->executePostedLayout();
    if (column < 0 || column >= d->header->count())
        return;
    int contents = sizeHintForColumn(column);
    int header = d->header->isHidden() ? 0 : d->header->sectionSizeHint(column);
    d->header->resizeSection(column, qMax(contents, header));
}

/*!
  Sorts the model by the values in the given \a column.
 */
void QTreeView::sortByColumn(int column)
{
    Q_D(QTreeView);
    if (!d->model)
        return;
    bool ascending = (header()->sortIndicatorSection() == column
                      && header()->sortIndicatorOrder() == Qt::DescendingOrder);
    Qt::SortOrder order = ascending ? Qt::AscendingOrder : Qt::DescendingOrder;
    header()->setSortIndicator(column, order);
    d->model->sort(column, order);
}

/*!
    Selects all the items in the underlying model.
*/
void QTreeView::selectAll()
{
    Q_D(QTreeView);
    if (!selectionModel())
        return;
    d->select(0, d->viewItems.count() - 1,
              QItemSelectionModel::ClearAndSelect
              |QItemSelectionModel::Rows);
}

/*!
    This function is called whenever \a{column}'s size is changed in
    the header. \a oldSize and \a newSize give the previous size and
    the new size in pixels.
*/
void QTreeView::columnResized(int column, int /* oldSize */, int /* newSize */)
{
    Q_D(QTreeView);
    d->columnsToUpdate.append(column);
    if (d->columnResizeTimerID == 0)
        d->columnResizeTimerID = startTimer(0);
}

/*!
  Updates the items in the tree view.
  \internal
*/
void QTreeView::updateGeometries()
{
    Q_D(QTreeView);
    QSize hint = d->header->isHidden() ? QSize(0, 0) : d->header->sizeHint();
    setViewportMargins(0, hint.height(), 0, 0);

    QRect vg = d->viewport->geometry();
    QRect geometryRect(vg.left(), vg.top() - hint.height(), vg.width(), hint.height());
    d->header->setGeometry(geometryRect);
    d->header->setOffset(horizontalScrollBar()->value());
    if (d->header->isHidden())
        QMetaObject::invokeMethod(d->header, "updateGeometries");

    if (model()) {
        d->updateVerticalScrollbar();
        d->updateHorizontalScrollbar();
    }

    QAbstractItemView::updateGeometries();
}

/*!
  Returns the size hint for the \a column's width or -1 if there is no
  model.

  If you need to set the width of a given column to a fixed value, call
  QHeaderView::resizeSection() on the view's header.

  If you reimplement this function in a subclass, note that the value you
  return is only used when resizeColumnToContents() is called. In that case,
  if a larger column width is required by either the view's header or
  the item delegate, that width will be used instead.

  \sa QWidget::sizeHint, header()
*/
int QTreeView::sizeHintForColumn(int column) const
{
    Q_D(const QTreeView);
    if (d->viewItems.isEmpty())
        return -1;
    int w = 0;
    QStyleOptionViewItem option = viewOptions();
    QAbstractItemDelegate *delegate = itemDelegate();
    const QVector<QTreeViewItem> viewItems = d->viewItems;
    for (int i = 0; i < viewItems.count(); ++i) {
        QModelIndex index = viewItems.at(i).index;
        if (index.column() != column)
            index = index.sibling(index.row(), column);
        int width = delegate->sizeHint(option, index).width();
        w = qMax(w, width + (column == 0 ? d->indentation(i) : 0));
    }
    return w;
}

/*!
  Returns the size hint for the row indicated by \a index.

  \sa sizeHintForColumn(), uniformRowHeights()
*/
int QTreeView::indexRowSizeHint(const QModelIndex &index) const
{
    Q_D(const QTreeView);
    if (!index.isValid() || !model())
        return -1;

    int start = -1;
    int end = -1;
    int count = d->header->count();
    if (count) {
        // If the sections have moved, we end up checking too many or too few
        start = d->header->logicalIndexAt(0);
        end = d->header->logicalIndexAt(viewport()->width());
    } else {
        // If the header has not been layed out yet, we use the model directly
        count = model()->columnCount(index.parent());
    }

    if (isRightToLeft()) {
        start = (start == -1 ? count - 1 : start);
        end = (end == -1 ? 0 : end);
    } else {
        start = (start == -1 ? 0 : start);
        end = (end == -1 ? count - 1 : end);
    }

    int tmp = start;
    start = qMin(start, end);
    end = qMax(tmp, end);

    int height = -1;
    QStyleOptionViewItem option = viewOptions();
    // This is temporary hack to speed up the function and will go away in 4.2
    option.rect.setWidth(-1);
    QAbstractItemDelegate *delegate = itemDelegate();
    for (int column = start; column <= end; ++column) {
        QModelIndex idx = index.sibling(index.row(), column);
        if (idx.isValid()) {
            if (QWidget *editor = d->editorForIndex(idx))
                height = qMax(height, editor->size().height());
            height = qMax(height, delegate->sizeHint(option, idx).height());
        }
    }

    return height;
}

/*!
  \reimp
*/
void QTreeView::horizontalScrollbarAction(int action)
{
    QAbstractItemView::horizontalScrollbarAction(action);
}

/*!
  \reimp
*/
bool QTreeView::isIndexHidden(const QModelIndex &index) const
{
    return (isColumnHidden(index.column()) || isRowHidden(index.row(), index.parent()));
}

/*
  private implementation
*/
void QTreeViewPrivate::initialize()
{
    Q_Q(QTreeView);
    q->setSelectionBehavior(QAbstractItemView::SelectRows);
    q->setSelectionMode(QAbstractItemView::SingleSelection);

    QHeaderView *header = new QHeaderView(Qt::Horizontal, q);
    header->setMovable(true);
    header->setStretchLastSection(true);
    q->setHeader(header);
}

void QTreeViewPrivate::expand(int i, bool emitSignal)
{
    Q_Q(QTreeView);

    if (!model || i == -1 || viewItems.at(i).expanded)
        return;

    q->setState(QAbstractItemView::ExpandingState);

    QModelIndex index = viewItems.at(i).index;
    expandedIndexes.append(index);

    viewItems[i].expanded = true;
    layout(i);

    // make sure we expand children that were previously expanded
    if (model->hasChildren(index))
        reexpandChildren(index);

    if (exactScrollbarPositions && i < topItemIndex)
        calculateTopItemIndex();

    q->setState(QAbstractItemView::NoState);

    if (emitSignal)
        emit q->expanded(index);
}

void QTreeViewPrivate::collapse(int item, bool emitSignal)
{
    Q_Q(QTreeView);

    if (!model || item == -1 || expandedIndexes.isEmpty())
        return;

    q->setState(QAbstractItemView::CollapsingState);

    int total = viewItems.at(item).total;
    QModelIndex modelIndex = viewItems.at(item).index;
    int index = expandedIndexes.indexOf(modelIndex);
    if (index == -1 || viewItems.at(item).expanded == false)
        return; // nothing to do

    expandedIndexes.remove(index);
    viewItems[item].expanded = false;

    index = item;
    QModelIndex parent = modelIndex;
    while (parent.isValid() && parent != root) {
        Q_ASSERT(index > -1);
        viewItems[index].total -= total;
        parent = parent.parent();
        index = viewIndex(parent);
    }
    viewItems.remove(item + 1, total); // collapse

    if (exactScrollbarPositions && item < topItemIndex)
        calculateTopItemIndex();
    
    q->setState(QAbstractItemView::NoState);

    if (emitSignal)
        emit q->collapsed(modelIndex);
}

void QTreeViewPrivate::layout(int i)
{
    Q_Q(QTreeView);
    QModelIndex current;
    QModelIndex parent = modelIndex(i);

    int count = 0;
    if (model->hasChildren(parent))
        count = model->rowCount(parent);

    if (i == -1)
        viewItems.resize(count);
    else
        viewItems.insert(i + 1, count, QTreeViewItem()); // expand

    int first = i + 1;
    int level = (i >= 0 ? viewItems.at(i).level + 1 : 0);
    int hidden = 0;
    int last = 0;

    int firstColumn = 0;
    while (q->isColumnHidden(firstColumn) && firstColumn < q->header()->count())
        ++firstColumn;

    for (int j = first; j < first + count; ++j) {
        current = model->index(j - first, firstColumn, parent);
        if (q->isRowHidden(current.row(), parent)) { // slow with lots of hidden rows
            ++hidden;
            last = j - hidden;
        } else {
            last = j - hidden;
            viewItems[last].index = current;
            viewItems[last].level = level;
        }
    }

    // remove hidden items
    if (hidden > 0)
        viewItems.remove(last + 1, hidden); // collapse

    QModelIndex root = q->rootIndex();
    while (parent != root) {
        Q_ASSERT(i > -1);
        viewItems[i].total += count - hidden;
        parent = parent.parent();
        i = viewIndex(parent);
    }
}

int QTreeViewPrivate::pageUp(int i) const
{
    int index = item(coordinate(i) - viewport->height());
    return index == -1 ? 0 : index;
}

int QTreeViewPrivate::pageDown(int i) const
{
    int index = item(coordinate(i) + viewport->height());
    return index == -1 ? viewItems.count() - 1 : index;
}

int QTreeViewPrivate::indentation(int i) const
{
    if (i < 0 || i >= viewItems.count())
        return 0;
    int level = viewItems.at(i).level;
    if (rootDecoration)
        ++level;
    return level * indent;
}

/*!
  \internal
  Returns the y coordinate for item
  Note: if this is ever changed to not estimate then update item()
*/
int QTreeViewPrivate::coordinate(int item) const
{
    Q_Q(const QTreeView);
    int scrollbarValue = q->verticalScrollBar()->value();
    int topViewItemIndex = itemAt(scrollbarValue); // first item (may start above the page)
    if (topViewItemIndex == -1) {
        const_cast<QTreeViewPrivate*>(this)->updateScrollbars();
        scrollbarValue = q->verticalScrollBar()->value();
        topViewItemIndex = itemAt(scrollbarValue);
    }
    Q_ASSERT(topViewItemIndex != -1);
    if( exactScrollbarPositions ) {
        int index = 0;
        int pos = 0;
        int step = 1;
        int itemCount = viewItems.size();
        if( item > topViewItemIndex / 2 ) {
            index = topViewItemIndex;
            pos = scrollbarValue - topItemOffset;
            if( item < topViewItemIndex )
                step = -1;
        }
        for( ; index >= 0 && index < itemCount; index += step ) {
            if( item == index ) {
                printf( "QTreeViewPrivate::coordinate(item = %i) returning %i\n", item, pos - scrollbarValue );
                return pos - scrollbarValue;
            }
            int h = height(index);
            pos += h * step;
        }
    } else {
        int viewItemIndex = topViewItemIndex; // first item (may start above the page)
        int viewItemHeight = height(viewItemIndex);
        int topItemCoordinate = topItemDelta(scrollbarValue, viewItemHeight);
        int viewItemCoordinate = topItemCoordinate;
        int viewportHeight = viewport->height();
        if (viewItemIndex <= item) {
            // search in the visible area first
            while (viewItemCoordinate < viewportHeight && viewItemIndex < viewItems.count()) {
                if (viewItemIndex == item)
                    return viewItemCoordinate; // item is visible - actual y in viewport
                viewItemCoordinate += height(viewItemIndex);
                ++viewItemIndex;
            }
            // the item is below the viewport
            if (editors.isEmpty()) { // optimized; estimate the coordinate
                return viewItemCoordinate + (itemHeight * (item - viewItemIndex));
            } else { // non-optimized
                for (;viewItemIndex < viewItems.count(); ++viewItemIndex) {
                    if (viewItemIndex == item)
                        return viewItemCoordinate;
                    viewItemCoordinate += height(viewItemIndex);
                }
            }
        }

        // the item is above the viewport
        if (editors.isEmpty()) { // optimized; estimate the coordinate
            return viewItemCoordinate - (itemHeight * (viewItemIndex - item));
        } else { // non-optimized
            viewItemCoordinate = topItemCoordinate;
            viewItemIndex = topViewItemIndex;
            for (; viewItemIndex >= 0; --viewItemIndex) {
                if (viewItemIndex == item)
                    return viewItemCoordinate;
                viewItemCoordinate -= height(viewItemIndex);
            }
        }
    }
    Q_ASSERT(false);
    return 0xDEADBEAF; // the item was not found
}

/*!
  \internal
  Returns the visual index at \a coordinate or -1
  \sa modelIndex()
*/
int QTreeViewPrivate::item(int yCoordinate) const
{
    Q_Q(const QTreeView);
    if( exactScrollbarPositions ) {
        int ret =itemAt(q->verticalScrollBar()->value() + yCoordinate);
        printf( "QTreeViewPrivate::item( yCoordinate = %i ) returning %i\n", yCoordinate, ret );
        return ret;
    }
    int scrollbarValue = q->verticalScrollBar()->value();
    int viewItemIndex = itemAt(scrollbarValue);
    if (viewItemIndex < 0) // couldn't find first visible item
        return -1;

    int viewItemHeight = height(viewItemIndex);
    int viewportHeight = viewport->height();
    int y = topItemDelta(scrollbarValue, viewItemHeight);
    if (yCoordinate >= y) {
        // search for item in viewport
        while (y < viewportHeight && viewItemIndex < viewItems.count()) {
            y += height(viewItemIndex);
            if (yCoordinate < y)
                return viewItemIndex;
            ++viewItemIndex;
        }
    }
    if (itemHeight <= 0)
        return -1;

    // item is above the viewport - find via binary search
    // If coordinate() is ever changed to not estimate
    // the value then change this to just walk the tree
    int start = 0;
    int end = viewItems.count();
    int i = (start + end + 1) >> 1;
    while (end - start > 0) {
        if (yCoordinate < coordinate(i))
            end = i - 1;
        else
            start = i;
        i = (start + end + 1) >> 1;
    }
    return (i < viewItems.count() ? i : -1);
}

int QTreeViewPrivate::viewIndex(const QModelIndex &index) const
{
    Q_Q(const QTreeView);
    if (!index.isValid())
        return -1;

    int totalCount = viewItems.count();
    QModelIndex parent = index.parent();

    // A quick check near the last item to see if we are just incrimenting
    int start = lastViewedItem > 2 ? lastViewedItem - 2 : 0;
    int end = lastViewedItem < totalCount - 2 ? lastViewedItem + 2 : totalCount;
    for (int i = start; i < end; ++i) {
        const QModelIndex idx = viewItems.at(i).index;
        if (idx.row() == index.row()) {
            if (idx.internalId() == index.internalId() || idx.parent() == parent) {// ignore column
                lastViewedItem = i;
                return i;
            }
        }
    }

    // NOTE: this function is slow if the item is outside the visible area
    // search in visible items first and below
    int t = itemAt(q->verticalScrollBar()->value());
    t = t > 100 ? t - 100 : 0; // start 100 items above the visible area

    for (int i = t; i < totalCount; ++i) {
        const QModelIndex idx = viewItems.at(i).index;
        if (idx.row() == index.row()) {
            if (idx.internalId() == index.internalId() || idx.parent() == parent) {// ignore column
                lastViewedItem = i;
                return i;
            }
        }
    }
    // search from top to first visible
    for (int j = 0; j < t; ++j) {
        const QModelIndex idx = viewItems.at(j).index;
        if (idx.row() == index.row()) {
            if (idx.internalId() == index.internalId() || idx.parent() == parent) { // ignore column
                lastViewedItem = j;
                return j;
            }
        }
    }
    // nothing found
    return -1;
}

QModelIndex QTreeViewPrivate::modelIndex(int i) const
{
    return ((i < 0 || i >= viewItems.count())
            ? (QModelIndex)root : viewItems.at(i).index);
}

int QTreeViewPrivate::itemAt(int value) const
{
    Q_Q(const QTreeView);
    if( exactScrollbarPositions ) {
        int pos = 0, item = 0;
        int totalCount = viewItems.count();
        if( value >= q->verticalScrollBar()->value() ) {
            pos = q->verticalScrollBar()->value() - topItemOffset;
            item = topItemIndex;
        }
        for( ; item < totalCount; ++item ) {
            int h = height(item);
            if( pos + h > value ) {
                //printf( "QTreeViewPrivate::itemAt( value = %i ) returning %i\n", value, item );
                return item;
            }
            pos += h;
        }
        return -1;
    } else {
        int i = value / verticalStepsPerItem;
        return (i < 0 || i >= viewItems.count()) ? -1 : i;
    }
}

void QTreeViewPrivate::calculateTopItemIndex()
{
    Q_Q(QTreeView);
    if( exactScrollbarPositions ) {
        int v = q->verticalScrollBar()->value();
        int pos = 0, item = 0;
        int totalCount = viewItems.count();
        for( ; item < totalCount; ++item ) {
            int h = height(item);
            if( pos + h > v ) {
                topItemOffset = v - pos;
                topItemIndex = item;
                return;
            }
            pos += h;
        }
    }
}

int QTreeViewPrivate::topItemDelta(int value, int iheight) const
{
    Q_Q(const QTreeView);
    if( exactScrollbarPositions ) {
        return -topItemOffset;//coordinate(itemAt(value));
    } else {
        int above = (value % verticalStepsPerItem) * iheight; // what's left; in "item units"
        return -(above / verticalStepsPerItem); // above the page
    }
}

int QTreeViewPrivate::columnAt(int x) const
{
    return header->logicalIndexAt(x);
}

void QTreeViewPrivate::relayout(const QModelIndex &parent)
{
    Q_Q(QTreeView);
    // do a local relayout of the items
    if (parent.isValid()) {
        int parentViewIndex = viewIndex(parent);
        if (parentViewIndex > -1 && viewItems.at(parentViewIndex).expanded) {
            collapse(parentViewIndex, false); // remove the current layout
            expand(parentViewIndex, false); // do the relayout
            q->updateGeometries();
            viewport->update();
        }
    } else {
        viewItems.clear();
        q->doItemsLayout();
    }
}

void QTreeViewPrivate::reexpandChildren(const QModelIndex &parent)
{
    if (!model)
        return;

    // FIXME: this is slow: optimize
    QVector<QPersistentModelIndex> o = expandedIndexes;
    for (int j = 0; j < o.count(); ++j) {
        QModelIndex index = o.at(j);
        if (!index.isValid()){
            int k = expandedIndexes.indexOf(index);
            if (k >= 0)
                expandedIndexes.remove(k);
        } else if (model->parent(index) == parent) {
            int v = viewIndex(index);
            if (v < 0)
                continue;
            int k = expandedIndexes.indexOf(index);
            expandedIndexes.remove(k);
            expand(v, false);
        }
    }
}

void QTreeViewPrivate::updateVerticalScrollbar()
{
    Q_Q(QTreeView);
    int viewHeight = viewport->height();
    int itemCount = viewItems.count();

    if( exactScrollbarPositions ) {
        int totalHeight = 0;
        for( int i=0; i<itemCount; i++ )
            totalHeight += height(i);
        q->verticalScrollBar()->setRange(0,totalHeight-viewHeight);
        q->verticalScrollBar()->setPageStep(viewHeight);
        calculateTopItemIndex();
    } else {
        // set page step size
        int verticalScrollBarValue = q->verticalScrollBar()->value();
        int itemsInViewport = 0;
        if (uniformRowHeights) {
            itemsInViewport = viewHeight / itemHeight;
        } else {
            int topItemInViewport = itemAt(verticalScrollBarValue);
            if (topItemInViewport < 0) {
                // if itemAt can't find the top item, there are no visible items in the view
                q->verticalScrollBar()->setRange(0, 0);
                q->verticalScrollBar()->setPageStep(0);
                return;
            }
            int h = height(topItemInViewport);
            int y = topItemDelta(verticalScrollBarValue, h);
            int i = topItemInViewport;
            for (; y < viewHeight && i < itemCount; ++i)
                y += height(i);
            itemsInViewport = i - topItemInViewport;
        }
        q->verticalScrollBar()->setPageStep(itemsInViewport * verticalStepsPerItem);

        // set the scroller range
        int y = viewHeight;
        int i = itemCount; // FIXME: wrong
        while (y > 0 && i > 0)
            y -= height(--i);
        int max = i * verticalStepsPerItem;

        if (y < 0) { // if the first item starts above the viewport, we have to backtrack
            int backtracking = verticalStepsPerItem * -y;
            int itemSize = height(i);
            if (itemSize > 0) // avoid division by zero
                max += (backtracking / itemSize) + 1;
        }

        q->verticalScrollBar()->setRange(0, max - 1);
    }
}

void QTreeViewPrivate::updateHorizontalScrollbar()
{
    Q_Q(QTreeView);
    int width = viewport->width();
    q->horizontalScrollBar()->setPageStep(width);
    q->horizontalScrollBar()->setRange(0, header->length() - width);
}

int QTreeViewPrivate::itemDecorationAt(const QPoint &pos) const
{
    Q_Q(const QTreeView);
    int x = pos.x();
    int column = header->logicalIndexAt(x);
    if (column == -1)
        return -1; // no logical index at x
    int position = header->sectionViewportPosition(column);
    int size = header->sectionSize(column);
    int cx = (q->isRightToLeft() ? size - x + position : x - position);
    int viewItemIndex = item(pos.y());
    int itemIndentation = indentation(viewItemIndex);
    QModelIndex index = modelIndex(viewItemIndex);

    if (!index.isValid() || column != 0
        || cx < (itemIndentation - indent) || cx > itemIndentation)
        return -1; // pos is outside the /ecoration rect

    if (!rootDecoration && index.parent() == q->rootIndex())
        return -1; // no decoration at root

    QRect rect;
    if (q->isRightToLeft())
        rect = QRect(position + size - itemIndentation, coordinate(viewItemIndex),
                     indent, height(viewItemIndex));
    else
        rect = QRect(position + itemIndentation - indent, coordinate(viewItemIndex),
                     indent, height(viewItemIndex));
    QStyleOption opt;
    opt.initFrom(q);
    opt.rect = rect;
    QRect returning = q->style()->subElementRect(QStyle::SE_TreeViewDisclosureItem, &opt, q);
    if (!returning.contains(pos))
        return -1;

    return viewItemIndex;
}

void QTreeViewPrivate::select(int top, int bottom,
                              QItemSelectionModel::SelectionFlags command)
{
    Q_Q(QTreeView);
    QModelIndex previous;
    QItemSelectionRange currentRange;
    QStack<QItemSelectionRange> rangeStack;
    QItemSelection selection;
    for (int i = top; i <= bottom; ++i) {
        QModelIndex index = modelIndex(i);
        QModelIndex parent = index.parent();
        if (previous.isValid() && parent == previous.parent()) {
            // same parent
            QModelIndex tl = model->index(currentRange.top(), currentRange.left(),
                                          currentRange.parent());
            currentRange = QItemSelectionRange(tl, index);
        } else if (previous.isValid() && parent == model->sibling(previous.row(), 0, previous)) {
            // item is child of previous
            rangeStack.push(currentRange);
            currentRange = QItemSelectionRange(index);
        } else {
            if (currentRange.isValid())
                selection.append(currentRange);
            if (rangeStack.isEmpty()) {
                currentRange = QItemSelectionRange(index);
            } else {
                currentRange = rangeStack.pop();
                if (parent == currentRange.parent()) {
                    QModelIndex tl = model->index(currentRange.top(),
                                                  currentRange.left(),
                                                  currentRange.parent());
                    currentRange = QItemSelectionRange(tl, index);
                } else {
                    selection.append(currentRange);
                    currentRange = QItemSelectionRange(index);
                }
            }
        }
        previous = index;
    }
    if (currentRange.isValid())
        selection.append(currentRange);
    for (int i = 0; i < rangeStack.count(); ++i)
        selection.append(rangeStack.at(i));
    q->selectionModel()->select(selection, command);
}

#endif // QT_NO_TREEVIEW
