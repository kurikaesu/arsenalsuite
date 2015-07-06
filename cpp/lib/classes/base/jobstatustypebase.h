#ifdef HEADER_FILES
#endif

#ifdef CLASS_FUNCTIONS
	bool operator==(const QString& rhs) const;
	bool operator!=(const QString& rhs) const;
	bool operator==(const Record& rhs) const;
	bool operator!=(const Record& rhs) const;
	
	static JobStatusType statusTypeByStatus(const QString& rhs);
#endif

#ifdef CLASS_PROTECTED

#endif

#ifdef CLASS_PRIVATES
	static QMap<QString, JobStatusType> pJobStatusTypes;
#endif

#ifdef TABLE_CTOR
#endif
