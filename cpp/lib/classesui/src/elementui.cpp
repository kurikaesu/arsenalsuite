
#include <qfile.h>

#include "iconserver.h"

#include "assettype.h"
#include "elementstatus.h"

#include "elementui.h"

ElementUi::ElementUi( const Element & e )
: mElement( e )
{}

QPixmap loadAndSize( const QString & fn, const QSize & size )
{
	QImage img( fn );
	if( img.isNull() ) {
		LOG_5( "Loading of " + fn + " failed" );
		return QPixmap();
	}
	if( img.width() > 48 )
		img = img.scaled( size, Qt::KeepAspectRatio, Qt::SmoothTransformation );
	return QPixmap::fromImage(img);
}

QPixmap ElementUi::image( const QSize & size ) const
{
	QString name;
	if( mElement.assetType().isRecord() )
		name = mElement.assetType().name().toLower() + ".png";
	else
		name = mElement.elementType().name().toLower() + ".png";

	name = name.replace( ' ', '_' );

	if( QFile::exists( "images/" + name ) )
		return loadAndSize( "images/" + name, size );
//    else
//		LOG_5( name + " not found in images/" );
	if( QFile::exists( ":/images/" + name ) )
		return loadAndSize( ":/images/" + name, size );

	if( mElement.isTask() )
		return QPixmap( ":/images/attach.png" );
	
	return icon( name );
}

QColor ElementUi::statusColor() const
{
	ElementStatus es( mElement.elementStatus() );
	if( mElement.isRecord() && !es.isRecord() ) {
		es = ElementStatus( 1 ); // New
		Element e(mElement);
		e.setElementStatus( es );
		e.commit();
	}
	QColor ret;
	ret.setNamedColor( es.color() );
	if( !ret.isValid() )
		return Qt::white;
	return ret;
}
