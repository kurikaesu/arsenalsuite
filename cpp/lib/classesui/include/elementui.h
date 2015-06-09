
#ifndef ELEMENT_UI_H
#define ELEMENT_UI_H

#include <qsize.h>

#include "element.h"

#include "classesui.h"

class QPixmap;
class QColor;

class CLASSESUI_EXPORT ElementUi
{
public:
	ElementUi( const Element & );

	/* Returns the thumbnail image, or default image for the element type */
	QPixmap image( const QSize & size = QSize( 16, 16 ) ) const;
	QColor statusColor() const;

protected:
	Element mElement;
};

#endif // ELEMENT_UI_H
