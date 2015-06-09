

#ifndef RANGE_FILE_TRACKER_FRAME
#define RANGE_FILE_TRACKER_FRAME

#include "rangefiletracker.h"

class RFTFrame {
public:
	RFTFrame();
	RFTFrame( const RangeFileTracker &, int frame );
	
	QString sortString() const;

	QString displayNumber() const;

	QString filePath() const;
	QString fileName() const;
	
	void fillFrame() const;
	void deleteFrame() const;

	bool exists() const;
		
	QString timeCode( int fps );
	
	bool isValid() const;
	
	RFTFrame & operator++();
	
	RangeFileTracker tracker;
	int frame;
};

typedef Q3ValueList<RFTFrame> RFTFrameList;
typedef Q3ValueListIterator<RFTFrame> RFTFrameIter;


#endif // RANGE_FILE_TRACKER_FRAME

