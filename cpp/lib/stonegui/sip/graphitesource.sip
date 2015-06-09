
class GraphiteDesc
{
%TypeHeaderCode
#include <graphitesource.h>
%End
public:
	enum AreaMode {
		None,
		First,
		All,
		Stacked
	};

	GraphiteDesc( QStringList sources = QStringList(), QSize size = QSize(640,480), AreaMode areaMode = None, const QDateTime & start = QDateTime(), const QDateTime & end = QDateTime(), int minValue = INT_MAX, int maxValue = INT_MAX );
	
	QUrl buildUrl( const QString & host = QString(), quint16 port = 80 ) const;
	static GraphiteDesc fromUrl( const QString & url );
	
	QStringList sources() const;
	void setSources( QStringList sources );
	
	QSize size() const;
	void setSize( const QSize & );

	AreaMode areaMode() const;
	void setAreaMode( AreaMode areaMode );
	
	QDateTime start() const;
	QDateTime end() const;
	void setDateRange( const QDateTime & start, const QDateTime & end );
	
	void setRelativeStart( const Interval & relativeStart );
	void setRelativeEnd( const Interval & relativeEnd );

	int minValue() const;
	int maxValue() const;
	void setValueRange( int minValue = INT_MAX, int maxValue = INT_MAX );
	void setMinValue( int minValue = INT_MAX );
	void setMaxValue( int maxValue = INT_MAX );

	QList<QPair<QString,QString> > extraQueryItems() const;
	void setExtraQueryItems( QList<QPair<QString,QString> > extra );
	
	void appendExtra( SIP_PYOBJECT );
%MethodCode
	if( PySequence_Check( a0 ) && PySequence_Size( a0 ) == 2 ) {
		PyObject * pyfirst = PySequence_GetItem( a0, 0 ), * pysecond = PySequence_GetItem( a0, 1 );
		if( sipCanConvertToType(pyfirst, sipType_QString, 0) && sipCanConvertToType(pysecond, sipType_QString, 0) ) {
			int first_state, second_state, err = 0;
			QString * first = (QString*)sipConvertToType( pyfirst, sipType_QString, 0, SIP_NOT_NONE, &first_state, &err );
			QString * second = (QString*)sipConvertToType( pysecond, sipType_QString, 0, SIP_NOT_NONE, &second_state, &err );
			if( !err )
				sipCpp->appendExtra( qMakePair<QString,QString>(*first,*second) );
			sipReleaseType( first, sipType_QString, first_state );
			sipReleaseType( second, sipType_QString, second_state );
		} else
			sipIsErr = 1;
		Py_XDECREF(pyfirst);
		Py_XDECREF(pysecond);
	} else
		sipIsErr = 1;
	if( sipIsErr )
		PyErr_SetString(PyExc_ValueError, "appendExtra requires a sequence with two string values.");
%End

	QList<GraphiteDesc> generateTimeSeries( int count ) const;
};

class GraphiteSource : QObject
{
%TypeHeaderCode
#include <graphitesource.h>
%End
public:
	GraphiteSource( QObject * parent = 0 );
	~GraphiteSource();

	enum Status {
		Ready,
		Active
	};

	Status status() const;
	
	QString graphiteServer();
	quint16 graphitePort();

	void start( const QNetworkRequest & request );

signals:
	void error( const QString & errorMessage );

public slots:
	void setGraphiteServer( const QString & server );
	void setGraphitePort( quint16 port );
	void cancel();
};

class GraphiteImageSource : GraphiteSource
{
%TypeHeaderCode
#include <graphitesource.h>
%End
public:
	GraphiteImageSource( QObject * parent = 0 );
	~GraphiteImageSource();

signals:
	void getImageProgress( qint64 recieved, qint64 total );
	void getImageFinished( const QImage & image );

public slots:
	void getImage( const GraphiteDesc & );
};


class GraphiteGetResult
{
%TypeHeaderCode
#include <graphitesource.h>
#include <limits>
using namespace std;
%End
public:
	QString path;
	Interval interval;
	QList<QPair<QDateTime,double> > data;
%GetCode
	// Create the list.
	PyObject *l;

	if ((l = PyList_New(sipCpp->data.size())) == NULL) {
		PyErr_SetString(PyExc_ValueError, "Unable to create new py list for returning GraphiteGetResult.data");
	} else {
		// Set the list elements.
		for (int i = 0; i < sipCpp->data.size(); ++i)
		{
			QPair<QDateTime,double> dataPair = sipCpp->data[i];
			PyObject *pobj = PyTuple_New(2);
			PyTuple_SET_ITEM(pobj,0,sipConvertFromNewType( new QDateTime(dataPair.first), sipType_QDateTime, 0 ));
			// IF NaN
			if( dataPair.second != dataPair.second ) {
				Py_INCREF(Py_None);
				PyTuple_SET_ITEM(pobj,1,Py_None);
			} else
				PyTuple_SET_ITEM(pobj,1,PyFloat_FromDouble(dataPair.second));
			PyList_SET_ITEM(l, i, pobj);
		}
	}
	sipPy = l;
%End
%SetCode
	if( PyList_Check( sipPy ) ) {
		// Clear the data
		sipCpp->data = QList< QPair<QDateTime,double> >();
		int cnt = PyList_Size(sipPy);
		for( int i = 0; i < cnt; i++ ) {
			PyObject * tup = PyList_GET_ITEM( sipPy, i );
			if( PyTuple_Check( tup ) && PyTuple_Size(tup) == 2 ) {
				PyObject * pyDt = PyTuple_GET_ITEM( tup, 0 );
				PyObject * pyVal = PyTuple_GET_ITEM( tup, 1 );
				if( sipCanConvertToType( pyDt, sipType_QDateTime, SIP_NOT_NONE ) ) {
					if( pyVal == Py_None || PyFloat_Check(pyVal) || PyLong_Check(pyVal) || PyInt_Check(pyVal) ) {
						QDateTime dt;
						int state = 0;
						QDateTime * dtP = reinterpret_cast<QDateTime *>(sipConvertToType( pyDt, sipType_QDateTime, 0, SIP_NOT_NONE, &state, &sipErr ));
						dt = *dtP;
						sipReleaseType( dtP, sipType_QDateTime, state );
						double val;
						if( pyVal == Py_None )
							val = numeric_limits<double>::quiet_NaN();
						else if( PyFloat_Check(pyVal) )
							val = PyFloat_AsDouble(pyVal);
						else if( PyLong_Check(pyVal) )
							val = double(PyLong_AsLong(pyVal));
						else if( PyInt_Check(pyVal) )
							val = double(PyInt_AsLong(pyVal));
						sipCpp->data.append( qMakePair<QDateTime,double>( dt, val ) );
					} else {
						LOG_1( "Unable to convert value data" );
					}
				} else {
					LOG_1( "Unable to convert QDateTime data" );
				}
			}
		}
	} else {
		PyErr_SetString(PyExc_ValueError, "Setting GraphiteGetResult.data is not supported.");
		sipErr = 1;
	}
%End

	int findClosestDataPoint( const QDateTime & dt );
	double getAverage( const QDateTime & start, const QDateTime & end );
	GraphiteGetResult adjustToInterval( const Interval &, const QDateTime & start = QDateTime(), int count = -1 );
};

class GraphiteDataSource : GraphiteSource
{
%TypeHeaderCode
#include <graphitesource.h>
%End
public:
	GraphiteDataSource( QObject * parent = 0 );
	~GraphiteDataSource();

signals:
	void getDataResult( const GraphiteGetResult & );

public slots:
	void getData( const QString & path, const QDateTime & start, const QDateTime & end );
};
