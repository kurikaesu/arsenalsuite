
#ifndef SPEWER_H
#define SPEWER_H

#include <qobject.h>

#include "thread.h"

class Spewer : public QObject
{
Q_OBJECT
public:
	Spewer( QObject * parent = 0 );
	
public slots:
	void threadsAdded( ThreadList );
	
};


#endif // SPEWER_H

