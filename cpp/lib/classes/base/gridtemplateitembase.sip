

	enum {
		Status 				= 1,
		AssignedUsers 		= 2,
		CheckListSummary 	= 4,
		FileName 			= 8,
		DaysBid 			= 16,
		DaysEstimated 		= 32,
		DaysScheduled 		= 64,
		DaysSpentActual 	= 128,
		DaysSpent8Hour 		= 256,
		Dependencies		= 512
	};

	MappedRecordList checkListItems() const;
	void setCheckListItems( CheckListItemList );
	QString textFromElement( const Element & ) const;
	//QColor colorFromElement( const Element & ) const;
	


