
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
 * $Id: fieldlineedit.h 5411 2007-12-18 01:03:08Z brobison $
 */

#ifndef FIELD_LINE_EDIT_H
#define FIELD_LINE_EDIT_H

#include <qlineedit.h>
#include <qpointer.h>
#include <qstringlist.h>

#include "recordproxy.h"
#include "stonegui.h"

class QMenu;

class STONEGUI_EXPORT FieldLineEdit : public QLineEdit
{
Q_OBJECT

	Q_PROPERTY( QString field READ field WRITE setField )
public:

	FieldLineEdit( RecordProxy *, const QString & field, QWidget * );
	FieldLineEdit( QWidget * parent );

	void setField( const QString & field );
	QString field() const;

	void setProxy( RecordProxy * proxy );

	bool changed() const;
	void reset();
	
public slots:
	void slotRecordListChanged();
	void slotUpdateRecordList();

	void popupItemActivated( int id );

	void slotTextChanged( const QString & text );

protected:

	virtual QMenu * createMenu();

	QPointer<RecordProxy> mProxy;
	QString mField, mOrig;
	QStringList mValues;
	int mFirstPopupId;
	int mChanged;
};

#endif // FIELD_LINE_EDIT_H

