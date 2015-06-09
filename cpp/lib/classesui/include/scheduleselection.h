
#ifndef SCHEDULE_SELECTION_H
#define SCHEDULE_SELECTION_H

#include <qdatetime.h>
#include <qlist.h>

#include "classesui.h"

class ScheduleWidget;
class ScheduleEntry;

/*
 *  This class is used to represent the selection of a continuos range of cells
 *  The cells can cover more than 1 row.
 */
class CLASSESUI_EXPORT ScheduleCellSelection
{
public:
	ScheduleCellSelection( ScheduleWidget * widget = 0, int cellStart = -1, int cellEnd = 0 );

	bool isValid() const { return mScheduleWidget && mCellStart >= 0 && mCellEnd >= mCellStart; }
	
	bool operator==( const ScheduleCellSelection & ) const;

	QDate startDate() const;
	QDate endDate() const;
	int duration() const{ return startDate().daysTo(endDate()) + 1; }

	bool contains( int cell ) const;

	bool isAdjacent( const ScheduleCellSelection & other );
	bool merge( const ScheduleCellSelection & other );

	// Returns anywhere from 0 to 2 objects that
	// represent the selection with cell removed
	QList<ScheduleCellSelection> remove( int cell );

	int mCellStart, mCellEnd;
	ScheduleWidget * mScheduleWidget;
};

/*
 * This class is used to represent the selection of a ScheduleEntry over
 * a continuous range of cells
 */
class CLASSESUI_EXPORT ScheduleEntrySelection : public ScheduleCellSelection
{
public:
	ScheduleEntrySelection( ScheduleWidget * widget = 0, ScheduleEntry * entry = 0, int cellStart = -1, int cellEnd = 0 );
	ScheduleEntrySelection( ScheduleEntry * entry, const ScheduleCellSelection & );

	bool isValid() const { return mEntry != 0 && ScheduleCellSelection::isValid(); }
	
	bool operator==( const ScheduleEntrySelection & ) const;

	bool merge( const ScheduleEntrySelection & other );

	// Returns anywhere from 0 to 2 objects that
	// represent the selection with cell removed
	QList<ScheduleEntrySelection> remove( int cell );

	ScheduleEntry * mEntry;
};

/*
 * This class is used to represent the current state of all selections
 * in a ScheduleWidget, and to perform selection operations.
 */
class CLASSESUI_EXPORT ScheduleSelection
{
public:
	ScheduleSelection( ScheduleWidget * widget );

	enum SelectOperation {
		AddSelection,
		RemoveSelection,
		InvertSelection
	};

	void select( int cell, ScheduleEntry * entry, SelectOperation );
	bool isCellSelected( int cell );
	bool isEntrySelected( int cell, ScheduleEntry * entry );

	ScheduleCellSelection getCellSelection( int cell );
	ScheduleEntrySelection getEntrySelection( int cell, ScheduleEntry * entry );
	
	void clear();

	QList<ScheduleCellSelection> mCellSelections;
	QList<ScheduleEntrySelection> mEntrySelections;
	ScheduleWidget * mScheduleWidget;
};

/*
 * Inline functions
 */ 
inline bool ScheduleCellSelection::operator==( const ScheduleCellSelection & other ) const
{
	return 
		( other.mCellStart == mCellStart )
	 && ( other.mCellEnd == mCellEnd );
}

inline bool ScheduleEntrySelection::operator==( const ScheduleEntrySelection & other ) const
{
	return mEntry == other.mEntry && ScheduleCellSelection::operator==(other);
}

#endif // SCHEDULE_SELECTION_H

