
class ResultSet
{
%TypeHeaderCode
#include <resultset.h>
%End

public:
	ResultSet(const QList<RecordList> & results);

//	ResultSetIter begin() const;
//	ResultSetIter end() const;

	int rows() const;
	int columns() const;

    int __len__();
%MethodCode
	sipRes = sipCpp->rows();
%End

	SIP_PYTUPLE operator[](int i);
%MethodCode
	int len;
	
	len = sipCpp->rows();
	
	if ((a0 = sipConvertFromSequenceIndex(a0, len)) < 0)
		sipIsErr = 1;
	else {
		RecordList row = sipCpp->row(a0);
		sipRes = PyTuple_New(row.size());
		for( int i=0; i<row.size(); ++i ) {
			PyObject * o = sipConvertFromNewType( new Record(row[i]), sipType_MappedRecord, 0 );
			PyTuple_SET_ITEM( sipRes, i, o );
		}
	}
%End

	RecordList column(int column) const;

	RecordList row(int row) const;

	Record at(int row, int column) const;
	
	QList<RecordList> data() const;
};

/*
class ResultSetIter
{
public:
	ResultSet resultSet() const;
//	RecordList operator*() const;
	RecordList row() const;
	
	Record column(int column);
	
	bool isValid() const;
	ResultSetIter next() const;
	ResultSetIter previous() const;
};
*/