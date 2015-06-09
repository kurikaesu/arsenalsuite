
enum JoinType {
	InnerJoin,
	LeftJoin,
	RightJoin,
	OuterJoin
};

class JoinCondition
{
%TypeHeaderCode
#include <joinedselect.h>
%End
public:
	QString condition;
	JoinType type;
	Table * table;
	bool ignoreResults;
};

// QList<JoinCondition> is implemented as a Python list of JoinConditions.
%MappedType QList<JoinCondition>
{
%TypeHeaderCode
#include <qlist.h>
%End

%ConvertFromTypeCode
    // Create the list.
    PyObject *l;

    if ((l = PyList_New(sipCpp->size())) == NULL)
        return NULL;

    // Set the list elements.
    for (int i = 0; i < sipCpp->size(); ++i)
    {
		JoinCondition rl = sipCpp->value(i);

		PyList_SET_ITEM(l, i, sipConvertFromNewType( new JoinCondition(rl), sipType_JoinCondition, NULL ) );
    }

    return l;
%End

%ConvertToTypeCode
    SIP_SSIZE_T len;

    // Check the type if that is all that is required.
	if (sipIsErr == NULL)
	{
		if (!PySequence_Check(sipPy) || (len = PySequence_Size(sipPy)) < 0)
			return 0;

		for (SIP_SSIZE_T i = 0; i < len; ++i)
		{
			PyObject *itm = PySequence_ITEM(sipPy, i);
			bool ok = (itm && sipCanConvertToType(itm, sipType_JoinCondition, SIP_NOT_NONE));

			Py_XDECREF(itm);

			if (!ok)
				return 0;
		}

		return 1;
	}

	QList<JoinCondition> *ql = new QList<JoinCondition>;
	len = PySequence_Size(sipPy);

	for (SIP_SSIZE_T i = 0; i < len; ++i)
	{
		PyObject *itm = PySequence_ITEM(sipPy, i);
		int state;
		JoinCondition * t = reinterpret_cast<JoinCondition*>(sipConvertToType(itm, sipType_JoinCondition, sipTransferObj, SIP_NOT_NONE, &state, sipIsErr));

		Py_DECREF(itm);

		if (*sipIsErr)
		{
			sipReleaseType(t, sipType_JoinCondition, state);

			delete ql;
			return 0;
		}

		ql->append(*t);

		sipReleaseType(t, sipType_JoinCondition, state);
	}

	*sipCppPtr = ql;

	return sipGetState(sipTransferObj);
%End
};

class JoinedSelect
{
%TypeHeaderCode
#include <joinedselect.h>
%End
public:
	JoinedSelect(Table * primaryTable, QList<JoinCondition> joinConditions = QList<JoinCondition>());
	JoinedSelect(Table * primaryTable, const QString & alias);

	QString alias() const;
	Table * table() const;
	QList<JoinCondition> joinConditions() const;

	JoinedSelect & join( SIP_PYOBJECT table, const QString & condition = QString(), JoinType type = InnerJoin, bool ignoreResults = false, const QString & alias = QString() );
%MethodCode
		// Convert to python
		Table * inst = 0;

		if( !sipCanConvertToType(a0,sipType_Table,SIP_NOT_NONE) ) {
			// Attempt to get the table * by calling the static method table()
			if( !sipIsErr && !inst ) {
				PyObject * pyTableMethod = PyObject_GetAttrString( a0, "table" );
				if( pyTableMethod ) {
					PyObject * pyTable = PyObject_CallObject( pyTableMethod, 0 );
					if( pyTable && sipCanConvertToType(pyTable,sipType_Table,SIP_NOT_NONE) )
						inst = (Table*)sipConvertToType(pyTable,sipType_Table,0,SIP_NOT_NONE,0,&sipIsErr);
					Py_XDECREF(pyTable);
				} else
					sipIsErr = 1;
			}
		}
		
		if( !inst && !sipIsErr && a0 && sipCanConvertToType(a0,sipType_Table,SIP_NOT_NONE) ) {
			inst = (Table*)sipConvertToType(a0,sipType_Table,0,SIP_NOT_NONE,0,&sipIsErr);
		}

		if( !sipIsErr ) {
			sipCpp->join( inst, *a1, a2, a3, *a4 );
			sipRes = sipCpp;
		}
%End

	ResultSet select(const QString & where = QString(), QList<QVariant> args = QList<QVariant>()) throw(SqlException,LostConnectionException,PythonException);
};
