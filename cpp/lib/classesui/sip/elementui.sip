
class ElementUi
{
%TypeHeaderCode
#include <elementui.h>
%End
public:
	ElementUi( const Element & );

	QPixmap image( const QSize & size = QSize( 16, 16 ) ) const;
	QColor statusColor() const;
};
