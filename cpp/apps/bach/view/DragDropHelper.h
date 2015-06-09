//-------------------------------------------------------------------------------------------------
/*
 * DragDropHelper.h
 *
 *  Created on: Jun 23, 2009
 *      Author: david.morris
 */

#ifndef DRAGDROPHELPER_H_
#define DRAGDROPHELPER_H_

#include <qdrag.h>
#include <qapplication.h>
#include "recorddrag.h"
#include "utils.h"

template< class T >
class DragDropHelper
{
public:
//-------------------------------------------------------------------------------------------------
	DragDropHelper( T * inst )
	:	mInst( inst )
	,	mDragStartPosition( -1, -1 )
	{}
//-------------------------------------------------------------------------------------------------
    bool mousePressEvent( QMouseEvent *event )
    {
        if (event->button() == Qt::LeftButton)
        {
            mDragStartPosition = event->pos();
        }
    	return false;
    }
//-------------------------------------------------------------------------------------------------
	bool mouseReleaseEvent( QMouseEvent *event )
	{
		if (event->button() == Qt::LeftButton)
			mSelection = BachAssetList();
		return false;
	}
//-------------------------------------------------------------------------------------------------
    bool mouseMoveEvent( QMouseEvent *event )
    {
        if (!(event->buttons() & Qt::LeftButton))
        {
        	DEBG( "No left button" );
            return false;
        }
        if ((event->pos() - mDragStartPosition).manhattanLength() < QApplication::startDragDistance())
        {
        	DEBG( "Not far enough for drag" );
            return true;
        }

    	DEBG( "Dragging..." );
        QDrag * drag = new QDrag( mInst );
        mSelection = mInst->selection();
        QMimeData * mimeData = RecordDrag::toMimeData( mSelection );
        drag->setMimeData( mimeData );

        /*Qt::DropAction dropAction =*/ drag->exec(Qt::MoveAction);
        return true;
    }
private:
	T * mInst;
	QPoint mDragStartPosition;
	BachAssetList mSelection;
};

#endif /* DRAGDROPHELPER_H_ */
