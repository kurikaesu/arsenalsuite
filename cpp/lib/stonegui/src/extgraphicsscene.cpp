
#include "qtooltip.h"
#include "qgraphicsproxywidget.h"
#include "qgraphicssceneevent.h"
#include "qgraphicsview.h"

#include "extgraphicsscene.h"

ExtGraphicsScene::ExtGraphicsScene(QObject *parent)
: QGraphicsScene(parent)
{}

ExtGraphicsScene::ExtGraphicsScene(const QRectF &sceneRect, QObject *parent)
: QGraphicsScene( sceneRect, parent )
{}

ExtGraphicsScene::ExtGraphicsScene(qreal x, qreal y, qreal width, qreal height, QObject *parent)
: QGraphicsScene(x, y, width, height, parent)
{}
	
ExtGraphicsScene::~ExtGraphicsScene()
{}

void ExtGraphicsScene::helpEvent(QGraphicsSceneHelpEvent *helpEvent)
{
	QList<QGraphicsItem *> itemsAtPos;

	QPointF scenePos = helpEvent->scenePos();
	QWidget * widget = helpEvent->widget();
	QGraphicsView * view = widget ? qobject_cast<QGraphicsView *>(widget->parentWidget()) : 0;

	if( view ) {
		const QRectF pointRect(scenePos, QSizeF(1, 1));
		if (view->isTransformed()) {
			const QTransform viewTransform = view->viewportTransform();
			itemsAtPos = items(pointRect, Qt::IntersectsItemShape, Qt::DescendingOrder, viewTransform);
		} else
			itemsAtPos = items(pointRect, Qt::IntersectsItemShape, Qt::DescendingOrder);
	} else
		itemsAtPos = items(scenePos, Qt::IntersectsItemShape, Qt::DescendingOrder, QTransform());

	QGraphicsItem *toolTipItem = 0;
	for (int i = 0; i < itemsAtPos.size(); ++i) {
		QGraphicsItem *tmp = itemsAtPos.at(i);
		if (tmp->type() == QGraphicsProxyWidget::Type) {
			// if the item is a proxy widget, the event is forwarded to it
			sendEvent(tmp, helpEvent);
			if (helpEvent->isAccepted())
				return;
		}
		if (tmp->toolTip().isEmpty())
			emit prepareToolTip(tmp);
			
		if (!tmp->toolTip().isEmpty()) {
			toolTipItem = tmp;
			break;
		}
	}

	// Show or hide the tooltip
	QString text;
	QPoint point;
	if (toolTipItem && !toolTipItem->toolTip().isEmpty()) {
		text = toolTipItem->toolTip();
		point = helpEvent->screenPos();
	}
	QToolTip::showText(point, text, helpEvent->widget());
	helpEvent->setAccepted(!text.isEmpty());
}
