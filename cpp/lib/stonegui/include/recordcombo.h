
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of libstone.
 *
 * libstone is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libstone is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libstone; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id$
 */

#ifndef RECORD_COMBO_H
#define RECORD_COMBO_H

#include <qcombobox.h>

#include "recordsupermodel.h"
#include "stonegui.h"

namespace Stone {
class Table;
class Index;
class Record;
}
using namespace Stone;

class STONEGUI_EXPORT RecordCombo : public QComboBox
{
Q_OBJECT
public:
	RecordCombo( QWidget * parent );
	
	void setItems( RecordList rl );

	void setTable( Table * table );
	void setColumn( const QString & column );
	
	void setModel( RecordSuperModel * model );

	RecordSuperModel * model() const { return mModel; }

	Record current()
	{
		QModelIndex idx = mModel->index(currentIndex(), 0);
		return idx.isValid() ? mModel->getRecord(idx) : Record();
	}

public slots:
	void setCurrent( const Record & r );
	void refresh( bool checkIndex = false );

signals:
	void currentChanged( const Record & );
	void highlighted( const Record & );

protected slots:
	void slotCurrentChanged( int index );
	void slotHighlighted( int index );
	void showTip( const QString & );
	
protected:
	void paintEvent( QPaintEvent * );

	Table * mTable;
	Index * mIndex;
	QString mColumn;
	RecordSuperModel * mModel;

private:
	void setModel( QAbstractItemModel * ){}
};


#endif // RECORD_COMBO_H

