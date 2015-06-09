
%Module QVariantTest
%Import QtCore/QtCoremod.sip

%ModuleHeaderCode
#include <qvariant.h>
QList<QVariant> returnVariantList( const QList<QVariant> & varList );
%End

%ModuleCode
QList<QVariant> returnVariantList( const QList<QVariant> & varList )
{ return varList; }
%End

QList<QVariant> returnVariantList( const QList<QVariant> & varList );
