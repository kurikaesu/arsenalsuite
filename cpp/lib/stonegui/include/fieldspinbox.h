
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
 * $Id: fieldspinbox.h 5411 2007-12-18 01:03:08Z brobison $
 */

#ifndef FIELD_SPIN_BOX_H
#define FIELD_SPIN_BOX_H

#include <qspinbox.h>
#include <qpointer.h>
#include <qstringlist.h>

#include "recordproxy.h"
#include "stonegui.h"

class QMenu;
class QSpinWidget;

/**
 *   This spin box allows setting multiple values at once.
 *   If more than one value is set, the background will
 *   be shown darkened and the highest value is displayed.
 *   If the value is changed the background will go back
 *   to the normal color.
 *   The context menu will show a list of each of the values
 *   and also allow setting the spinbox back to "No Change".
 *
 *   This class is useful for editting multiple records
 *   simultaniously on a single form.
 */
class STONEGUI_EXPORT MultipleValueSpinBox : public QSpinBox
{
Q_OBJECT
public:
	MultipleValueSpinBox( QWidget * parent );

	void setValues( QList<int> values );
	/** Returns a list of current values, will always contain
	 *  the same number of items as originalValues().
	 *  If the value has been changed, each value will be equal
	 *  to value(), otherwise the values will be equivalent to
	 *  originalValues().
	 **/
	QList<int> values() const;

	/**
	 *   Returns the values last set by calling setValues.
	 **/
	QList<int> originalValues() const;

	/**
	 *  Returns true if the value has changed since the last
	 *  call to setValues.
	 **/
	bool changed() const;

	/**
	 *  If changed is true, all values will be updated to the
	 *  current value().  Else all values will be updated
	 *  to their original value.
	 */
	void setChanged( bool changed );

signals:
	void valueChanged( int value, bool changed );

protected slots:
	void slotValueChanged();

protected:
	void init();
	void emitValueChanged();
	void setMultipleValuePalette(bool);
	virtual QString textFromValue ( int value ) const;
	virtual int valueFromText ( const QString & text ) const;
	virtual void contextMenuEvent( QContextMenuEvent * event );
	QPalette multipleValuePalette();

	int mOrig;
	QList<int> mOrigValues, mUniqueValues;
	bool mChanged, mMultipleValuePaletteSet;

	// For mouse movement based value scrolling
	int mStartPos, mStartValue;
};

class STONEGUI_EXPORT FieldSpinBox : public MultipleValueSpinBox
{
Q_OBJECT
	Q_PROPERTY( QString field READ field WRITE setField )

public:
	FieldSpinBox( QWidget * parent );
	FieldSpinBox( RecordProxy *, const QString & field, QWidget * );

	void setField( const QString & field );
	QString field() const;

	void setProxy( RecordProxy * proxy );
	
public slots:
	void slotRecordListChanged();
	void slotUpdateRecordList();

protected:
	virtual bool eventFilter( QObject * o, QEvent * e );

	QPointer<RecordProxy> mProxy;
	QString mField;
};

#endif // FIELD_SPIN_BOX_H

