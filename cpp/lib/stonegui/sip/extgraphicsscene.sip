
class ExtGraphicsScene : QGraphicsScene
{
%TypeHeaderCode
#include <extgraphicsscene.h>
%End

public:
	ExtGraphicsScene(QObject *parent /TransferThis/ = 0);
	ExtGraphicsScene(const QRectF &sceneRect, QObject *parent /TransferThis/ = 0);
	ExtGraphicsScene(qreal x, qreal y, qreal width, qreal height, QObject *parent /TransferThis/ = 0);
	virtual ~ExtGraphicsScene();

signals:
	void prepareToolTip( QGraphicsItem * );

protected:
	virtual void helpEvent(QGraphicsSceneHelpEvent *helpEvent);
};
