
#ifndef EXT_GRAPHICS_SCENE_H
#define EXT_GRAPHICS_SCENE_H

#include <qgraphicsscene.h>

#include "stonegui.h"

class STONEGUI_EXPORT ExtGraphicsScene : public QGraphicsScene
{
Q_OBJECT
public:
	ExtGraphicsScene(QObject *parent = 0);
	ExtGraphicsScene(const QRectF &sceneRect, QObject *parent = 0);
	ExtGraphicsScene(qreal x, qreal y, qreal width, qreal height, QObject *parent = 0);
	virtual ~ExtGraphicsScene();

signals:
	void prepareToolTip( QGraphicsItem * );

protected:
	virtual void helpEvent(QGraphicsSceneHelpEvent *helpEvent);
};

#endif //  EXT_GRAPHICS_SCENE_H