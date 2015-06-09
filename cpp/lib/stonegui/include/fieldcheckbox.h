
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
 * $Id: fieldcheckbox.h 5411 2007-12-18 01:03:08Z brobison $
 */

#ifndef FIELD_CHECK_BOX_H
#define FIELD_CHECK_BOX_H

#include <qcheckbox.h>
#include <qpointer.h>

#include "stonegui.h"
#include "recordproxy.h"

/**
 *  This class is used to place checkboxes
 *  on forms, and have their value automatically
 *  updated to reflect the contents of the
 *  recordlist assigned to the recordproxy.
 *
 *  It can display/modify multiple records at
 *  once, and automatically goes into tristate
 *  mode if there are conflicting values.
 */
class STONEGUI_EXPORT FieldCheckBox : public QCheckBox
{
Q_OBJECT
	Q_PROPERTY( QString field READ field WRITE setField )

public:
	FieldCheckBox( QWidget * parent );

	/** Sets the record's field that is used to get/set the
	 *  value of the checkbox.
	 **/
	void setField( const QString & field );
	/** Returns the current field **/
	QString field() const;

	/** Sets the RecordProxy that contains the records
	 *  to display/edit */
	void setProxy( RecordProxy * proxy );
	
public slots:
	void slotRecordListChanged();
	void slotUpdateRecordList();

protected:
	QPointer<RecordProxy> mProxy;
	QString mField;
};


#endif // FIELD_CHECK_BOX_H

