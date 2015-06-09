
#include "scheduleselection.h"
#include "schedulewidget.h"

ScheduleCellSelection::ScheduleCellSelection( ScheduleWidget * widget, int cellStart, int cellEnd )
: mCellStart( cellStart )
, mCellEnd( qMax(cellStart,cellEnd) )
, mScheduleWidget( widget )
{
}

QDate ScheduleCellSelection::startDate() const
{
	int cc = qMax(1,mScheduleWidget->columnCount());
	return mScheduleWidget->dateAtCell( mCellStart / cc, mCellStart % cc );
}

QDate ScheduleCellSelection::endDate() const
{
	int cc = qMax(1,mScheduleWidget->columnCount());
	return mScheduleWidget->dateAtCell( mCellEnd / cc, mCellEnd % cc );
}

bool ScheduleCellSelection::contains( int cell ) const
{
	return cell >= mCellStart && cell <= mCellEnd;
}

bool ScheduleCellSelection::isAdjacent( const ScheduleCellSelection & other )
{
	if( mCellEnd + 1 == other.mCellStart ) return true;
	if( other.mCellEnd + 1 == mCellStart ) return true;
	return false;
}

bool ScheduleCellSelection::merge( const ScheduleCellSelection & other )
{
	if( isAdjacent( other ) ) {
		mCellStart = qMin( mCellStart, other.mCellStart );
		mCellEnd = qMax( mCellEnd, other.mCellEnd );
		return true;
	}
	return false;
}

// Returns anywhere from 0 to 2 objects that
// represent the selection with cell removed
QList<ScheduleCellSelection> ScheduleCellSelection::remove( int cell )
{
	QList<ScheduleCellSelection> ret;
	if( cell < mCellStart || cell > mCellEnd ) {
		ret += *this;
		return ret;
	}
	
	if( mCellStart < cell )
		ret += ScheduleCellSelection( mScheduleWidget, mCellStart, cell - 1 );
	
	if( mCellEnd > cell )
		ret += ScheduleCellSelection( mScheduleWidget, cell + 1, mCellEnd );
	
	return ret;
}


ScheduleEntrySelection::ScheduleEntrySelection( ScheduleWidget * widget, ScheduleEntry * entry, int cellStart, int cellEnd )
: ScheduleCellSelection( widget, cellStart, cellEnd )
, mEntry( entry )
{}

ScheduleEntrySelection::ScheduleEntrySelection( ScheduleEntry * entry, const ScheduleCellSelection & other )
: ScheduleCellSelection( other )
, mEntry( entry )
{}


bool ScheduleEntrySelection::merge( const ScheduleEntrySelection & other )
{
	 return other.mEntry == mEntry && ScheduleCellSelection::merge( other );
}

// Returns anywhere from 0 to 2 objects that
// represent the selection with cell removed
QList<ScheduleEntrySelection> ScheduleEntrySelection::remove( int cell )
{
	QList<ScheduleEntrySelection> ret;
	QList<ScheduleCellSelection> cr = ScheduleCellSelection::remove( cell );
	foreach( ScheduleCellSelection cs, cr )
		ret += ScheduleEntrySelection( mEntry, cs );
	return ret;
}

ScheduleSelection::ScheduleSelection( ScheduleWidget * widget )
: mScheduleWidget( widget )
{}

void ScheduleSelection::select( int cell, ScheduleEntry * entry, SelectOperation op )
{
	bool add = false;
	bool rem = false;
	bool sel = false;
	
	if( entry )
		sel = isEntrySelected( cell, entry );
	else
		sel = isCellSelected( cell );
	
	add = !sel && (op == AddSelection || op == InvertSelection);
	rem = sel && (op == RemoveSelection || op == InvertSelection);
	
	// Return if there is nothing to do
	if( !add && !rem ) return;
	
	if( entry ) {
		if( add ) {
			bool merged = false;
			ScheduleEntrySelection sel( mScheduleWidget, entry, cell );
			for( int i=0; i < mEntrySelections.size(); i++ ) {
				ScheduleEntrySelection & es( mEntrySelections[i] );
				if( es.merge( sel ) ) {
					// If we have already merged once, then only one more merge is possible
					if( merged ) {
						mEntrySelections.removeAll( sel );
						break;
					}
					// Start the loop over, the merged entry might need to be merged again
					merged = true;
					sel = es;
					i = -1;
				}
			}
			if( !merged )
				mEntrySelections += sel;
		} else {
			for( int i=0; i < mEntrySelections.size(); i++ ) {
				ScheduleEntrySelection & es( mEntrySelections[i] );
				if( es.mEntry == entry && es.contains( cell ) ) {
					mEntrySelections += es.remove(cell);
					mEntrySelections.removeAll(es);
					break;
				}
			}
		}
	} else {
		if( add ) {
			bool merged = false;
			ScheduleCellSelection sel( mScheduleWidget, cell );
			for( int i=0; i < mCellSelections.size(); i++ ) {
				ScheduleCellSelection & es( mCellSelections[i] );
				if( es.merge( sel ) ) {
					if( merged ) {
						mCellSelections.removeAll( sel );
						break;
					}
					merged = true;
					sel = es;
					i = -1;
				}
			}
			if( !merged )
				mCellSelections += sel;
		} else {
			for( int i=0; i < mCellSelections.size(); i++ ) {
				ScheduleCellSelection & es( mCellSelections[i] );
				if( es.contains( cell ) ) {
					mCellSelections += es.remove(cell);
					mCellSelections.removeAll(es);
					break;
				}
			}
		}
	}
}

bool ScheduleSelection::isCellSelected( int cell )
{
	foreach( ScheduleCellSelection cs, mCellSelections )
		if( cs.contains( cell ) ) return true;
	return false;
}

bool ScheduleSelection::isEntrySelected( int cell, ScheduleEntry * entry )
{
	foreach( ScheduleEntrySelection es, mEntrySelections )
		if( es.mEntry == entry && es.contains( cell ) ) return true;
	return false;
}

ScheduleCellSelection ScheduleSelection::getCellSelection( int cell )
{
	foreach( ScheduleCellSelection cs, mCellSelections )
		if( cs.contains( cell ) )
			return cs;
	return ScheduleCellSelection();
}

ScheduleEntrySelection ScheduleSelection::getEntrySelection( int cell, ScheduleEntry * entry )
{
	foreach( ScheduleEntrySelection es, mEntrySelections )
		if( es.mEntry == entry && es.contains( cell ) )
			return es;
	return ScheduleEntrySelection();
}

void ScheduleSelection::clear()
{
	mCellSelections.clear();
	mEntrySelections.clear();
}
