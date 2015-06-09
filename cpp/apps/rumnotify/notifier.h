

#ifndef NOTIFIER_H
#define NOTIFIER_H

#include <qobject.h>
#include <q3valuelist.h>

class Notifier : public QObject
{
Q_OBJECT
public:
	Notifier( const QString & command, const QString & table, const Q3ValueList<uint> & keys );

public slots:
	void slotUpdateManagerStatusChange( bool );

protected:

	QString mCommand, mTable;
	Q3ValueList<uint> mKeys;	
};


#endif // NOTIFIER_H

