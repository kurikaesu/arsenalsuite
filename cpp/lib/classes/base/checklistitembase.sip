
	MappedRecord status( const Element & ) const;

	void setStatus( const Element &, const ElementStatus & ) const;

	QString statusString( const Element & ) const;

	static QString summary( const Element & );

/*	static CheckListItemList list( const Element & );

	static CheckListItemList defaults( const TaskType &, const Project & p= Project() );

	static void setDefaults( CheckListItemList items, const TaskType &, const Project & p= Project() );

*/


