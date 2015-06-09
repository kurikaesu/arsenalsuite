
	enum {
		RequiresSignoff = 1,
		SignedOff = 2,
		HasBeenRead = 4
	};

	bool requiresSignoff() const;
	void setRequiresSignoff( bool rs );

	bool signedOff() const;
	void setSignedOff( bool );
	
	bool read() const;
	void setRead( bool );
	

