
#include <limits>
#include <math.h>

#include <qimage.h>
#include <qimagereader.h>
#include <qstringlist.h>

#include <qnetworkaccessmanager.h>

#include "blurqt.h"
#include "iniconfig.h"
#include "graphitesource.h"

using namespace std;

GraphiteDesc::GraphiteDesc( QStringList sources, QSize size, AreaMode areaMode, const QDateTime & start, const QDateTime & end, int minValue, int maxValue )
: mSources( sources )
, mSize( size )
, mAreaMode( areaMode )
, mStart( start )
, mEnd( end )
, mMinValue( minValue )
, mMaxValue( maxValue )
{}

GraphiteDesc::~GraphiteDesc()
{}

QString GraphiteDesc::areaModeToString( GraphiteDesc::AreaMode areaMode )
{
	if( areaMode == GraphiteDesc::First )
		return "first";
	if( areaMode == GraphiteDesc::All )
		return "all";
	if( areaMode == GraphiteDesc::Stacked )
		return "stacked";
	return "none";
}

GraphiteDesc::AreaMode GraphiteDesc::areaModeFromString( const QString & _areaModeStr )
{
	QString areaModeStr(_areaModeStr.toLower());
	if( areaModeStr == "first" )
		return GraphiteDesc::First;
	else if( areaModeStr == "all" )
		return GraphiteDesc::All;
	else if( areaModeStr == "stacked" )
		return GraphiteDesc::Stacked;
	return GraphiteDesc::None;
}

static QString intervalToGraphite( Interval intv )
{
	if( (intv.months() > 0 || intv.days() > 0) && intv.seconds() == 0 )
		return QString( "-%1d" ).arg( llabs(intv.asOrder(Interval::Days)) );
	qint64 seconds = intv.asOrder(Interval::Seconds);
	if( seconds % 60 == 0 )
		return QString( "-%1min" ).arg( llabs(seconds / 60) );
	return QString( "-%1s" ).arg( llabs(seconds) );
}

static Interval intervalFromGraphite( const QString & intStr )
{
	if( intStr.isEmpty() ) return Interval();
	char lc = intStr.at(intStr.size()-1).toAscii();
	int toChop = 1;
	// min,mon are 3 letters, we'll use i,o to distinguish
	if( lc == 'n' ) {
		toChop = 3;
		lc = intStr.at(intStr.size()-2).toAscii();
	}
	bool okay;
	int num = intStr.left(intStr.size()-toChop).toInt(&okay);
	if( !okay ) return Interval();
	switch( lc ) {
		case 's':
			return Interval(num);
		case 'i':
			return Interval(num*60);
		case 'h':
			return Interval(num*60*60);
		case 'd':
			return Interval(0,num,0);
		case 'o':
			return Interval().addMonths(num);
		case 'y':
			return Interval(num,0,0);
	}
	return Interval();
}

QUrl GraphiteDesc::buildUrl( const QString & host, quint16 port ) const
{
	QUrl ret;
	ret.setScheme("http");
	ret.setHost( host );
	if( port != 80 )
		ret.setPort( port );
	ret.setPath( "/render/" );
	if( mSize.isValid() ) {
		ret.addQueryItem( "width", QString::number(mSize.width()) );
		ret.addQueryItem( "height", QString::number(mSize.height()) );
	}
	foreach( QString source, mSources )
		ret.addQueryItem( "target", source );
	if( mMinValue != INT_MAX )
		ret.addQueryItem( "min", QString::number(mMinValue) );
	if( mMaxValue != INT_MAX )
		ret.addQueryItem( "max", QString::number(mMaxValue) );
	if( mStart.isValid() )
		ret.addQueryItem( "from", QString::number(mStart.toTime_t()) );
	else if( mRelativeStart != Interval() )
		ret.addQueryItem( "from", intervalToGraphite(mRelativeStart) );
	if( mEnd.isValid() )
		ret.addQueryItem( "until", QString::number(mEnd.toTime_t()) );
	else if( mRelativeEnd != Interval() )
		ret.addQueryItem( "until", intervalToGraphite(mRelativeEnd) );
	ret.addQueryItem( "areaMode", areaModeToString(mAreaMode) );
	QPair<QString,QString> pair;
	foreach( pair, mExtra )
		ret.addQueryItem( pair.first, pair.second );
	return ret;
}

GraphiteDesc GraphiteDesc::fromUrl( const QString & urlStr )
{
	GraphiteDesc ret;
	QUrl url(urlStr);
	QPair<QString,QString> pair;
	QSize size( 640, 480 );
	ret.setSources( url.allQueryItemValues( "target" ) );
	foreach( pair, url.queryItems() ) {
		QString key = pair.first, value = pair.second;
		if( key == "target" ) continue;
		if( key == "width" )
			size.setWidth( value.toInt() );
		else if( key == "height" )
			size.setHeight( value.toInt() );
		else if( key == "min" )
			ret.setMinValue( value.toInt() );
		else if( key == "max" )
			ret.setMaxValue( value.toInt() );
		else if( key == "from" ) {
			if( value.startsWith("-") )
				ret.setRelativeStart(intervalFromGraphite(value));
			else
				ret.setStart(QDateTime::fromString(value));
		}
		else if( key == "until" ) {
			if( value.startsWith("-") )
				ret.setRelativeEnd(intervalFromGraphite(value));
			else
				ret.setEnd(QDateTime::fromString(value));
		}
		else if( key == "areaMode" )
			ret.setAreaMode( areaModeFromString(value) );
		else
			ret.appendExtra( qMakePair<QString,QString>(key,value) );
	}
	ret.setSize( size );
	return ret;
}
	
QStringList GraphiteDesc::sources() const
{
	return mSources;
}

void GraphiteDesc::setSources( QStringList sources )
{
	mSources = sources;
}

QSize GraphiteDesc::size() const
{ return mSize; }

void GraphiteDesc::setSize( const QSize & size )
{
	mSize = size;
}

GraphiteDesc::AreaMode GraphiteDesc::areaMode() const
{ return mAreaMode; }

void GraphiteDesc::setAreaMode( AreaMode areaMode )
{
	mAreaMode = areaMode;
}

QDateTime GraphiteDesc::start() const { return mStart; }
QDateTime GraphiteDesc::end() const { return mEnd; }
Interval GraphiteDesc::relativeStart() const { return mRelativeStart; }
Interval GraphiteDesc::relativeEnd() const { return mRelativeEnd; }

void GraphiteDesc::setStart( const QDateTime & start )
{
	mStart = start;
	mRelativeStart = Interval();
}

void GraphiteDesc::setRelativeStart( const Interval & relativeStart )
{
	mRelativeStart = relativeStart;
	mStart = QDateTime();
}

void GraphiteDesc::setEnd( const QDateTime & end )
{
	mEnd = end;
	mRelativeEnd = Interval();
}

void GraphiteDesc::setRelativeEnd( const Interval & relativeEnd )
{
	mRelativeEnd = relativeEnd;
	mEnd = QDateTime();
}

void GraphiteDesc::setDateRange( const QDateTime & start, const QDateTime & end )
{
	mStart = start;
	mEnd = end;
}

int GraphiteDesc::minValue() const { return mMinValue; }
int GraphiteDesc::maxValue() const { return mMaxValue; }

void GraphiteDesc::setValueRange( int minValue, int maxValue )
{
	mMinValue = minValue;
	mMaxValue = maxValue;
}

void GraphiteDesc::setMinValue( int minValue )
{
	mMinValue = minValue;
}

void GraphiteDesc::setMaxValue( int maxValue )
{
	mMaxValue = maxValue;
}

QList<QPair<QString,QString> > GraphiteDesc::extraQueryItems() const
{
	return mExtra;
}

void GraphiteDesc::setExtraQueryItems( QList<QPair<QString,QString> > extra )
{
	mExtra = extra;
}

void GraphiteDesc::appendExtra( QPair<QString,QString> extra )
{
	mExtra.append( extra );
}

QList<GraphiteDesc> GraphiteDesc::generateTimeSeries( int count ) const
{
	QDateTime now( QDateTime::currentDateTime() );
	QDateTime start = mStart;
	if( !mStart.isValid() && mRelativeStart > 0 )
		start = (mRelativeStart * -1.0).adjust(now);
	if( !start.isValid() )
		start = Interval::fromString("-1 day").adjust(now);
	QDateTime end = mEnd;
	if( !mEnd.isValid() && mRelativeEnd > 0 )
		end = (mRelativeEnd * -1.0).adjust(now);
	if( !end.isValid() )
		end = now;
	Interval span(start,end);
	QList<GraphiteDesc> ret;
	ret += *this;
	bool hasRelativeStart = !mStart.isValid();
	bool hasRelativeEnd = !mEnd.isValid();
	for( int i = 0; i < count; i++ )
	{
		// Moving backward in time
		end = start;
		start = (span * -1.0).adjust(start);
		GraphiteDesc gd(*this);
		if( hasRelativeEnd )
			gd.mRelativeEnd = Interval(end,now);
		else
			gd.mEnd = end;
		if( hasRelativeStart )
			gd.mRelativeStart = Interval(start,now);
		else
			gd.mStart = start;
		ret += gd;
	}
	return ret;
}

int GraphiteSource::sNetworkAccessManagerRefCount = 0;
QNetworkAccessManager * GraphiteSource::sNetworkAccessManager = 0;


GraphiteSource::GraphiteSource( QObject * parent )
: QObject( parent )
, mCurrentRequest( 0 )
{
	IniConfig & ini = config();
	ini.pushSection( "Graphite" );
	mGraphiteServer = ini.readString( "WebHost", "graphite.blur.com" );
	mGraphitePort = ini.readInt( "WebPort", 80 );
	ini.popSection();
	
	if( !sNetworkAccessManager ) {
		sNetworkAccessManager = new QNetworkAccessManager();
		sNetworkAccessManagerRefCount = 1;
	} else
		sNetworkAccessManagerRefCount++;
}

GraphiteSource::~GraphiteSource()
{
	if( --sNetworkAccessManagerRefCount == 0 ) {
		delete sNetworkAccessManager;
		sNetworkAccessManager = 0;
	}
}

GraphiteSource::Status GraphiteSource::status() const
{
	return mCurrentRequest ? Active : Ready;
}

QString GraphiteSource::graphiteServer()
{
	return mGraphiteServer;
}

quint16 GraphiteSource::graphitePort()
{
	return mGraphitePort;
}

void GraphiteSource::setGraphiteServer( const QString & server )
{
	mGraphiteServer = server;
}

void GraphiteSource::setGraphitePort( quint16 port )
{
	mGraphitePort = port;
}

void GraphiteSource::cancel()
{
	if( mCurrentRequest ) {
		mCurrentRequest->disconnect( this );
		mCurrentRequest->deleteLater();
		mCurrentRequest = 0;
	}
}

void GraphiteSource::start( const QNetworkRequest & request )
{
	mCurrentRequest = sNetworkAccessManager->get( request );
	LOG_5( "Network Request Sent, URL: " + request.url().toString() );
	connect( mCurrentRequest, SIGNAL( error( QNetworkReply::NetworkError ) ), SLOT( networkRequestError( QNetworkReply::NetworkError ) ) );
	connect( mCurrentRequest, SIGNAL( finished() ), SLOT( networkRequestFinished() ) );
}

void GraphiteSource::networkRequestFinished()
{
	mCurrentRequest->deleteLater();
	mCurrentRequest = 0;
}

void GraphiteSource::networkRequestError( QNetworkReply::NetworkError errorCode )
{
	_error( "QNetworkReply returned error " + QString::number( errorCode ) );
	if( mCurrentRequest ) {
		mCurrentRequest->deleteLater();
		mCurrentRequest = 0;
	}
}

void GraphiteSource::_error( const QString & errorMessage )
{
	LOG_5( errorMessage );
	emit error( errorMessage );
}

GraphiteImageSource::GraphiteImageSource( QObject * parent )
: GraphiteSource(parent)
{
}

GraphiteImageSource::~GraphiteImageSource()
{
}

void GraphiteImageSource::getImage( const GraphiteDesc & data )
{
	QUrl url = data.buildUrl( mGraphiteServer, mGraphitePort );
	start( QNetworkRequest( url ) );
	if( mCurrentRequest )
		connect( mCurrentRequest, SIGNAL( downloadProgress( qint64, qint64 ) ), SIGNAL( getImageProgress( qint64, qint64 ) ) );
}

void GraphiteImageSource::networkRequestFinished()
{
	if( mCurrentRequest ) {
		foreach( QByteArray headerName, mCurrentRequest->rawHeaderList() ) {
			LOG_5( QString::fromLatin1(headerName) + ": " + QString::fromLatin1(mCurrentRequest->rawHeader(headerName)) );
		}
		
		// Should be image/png, if not let QImageReader guess the format
		QByteArray imageFormat;
		if( mCurrentRequest->header(QNetworkRequest::ContentTypeHeader).toString() == "image/png" )
			imageFormat = "PNG";
		
		QImageReader imageReader( mCurrentRequest, imageFormat );
		if( imageReader.canRead() ) {
			QImage image;
			if( imageReader.read(&image) ) {
				emit getImageFinished( image );
			} else {
				_error( "QImageReader::read failed to read image from QNetworkReply" );
			}
		} else {
			_error( "QImageReader cannot read the data stored in the QNetworkReply" );
		}
		mCurrentRequest->deleteLater();
		mCurrentRequest = 0;
	}
}


int GraphiteGetResult::findClosestDataPoint( const QDateTime & dt )
{
	int size = data.size();
	if( size == 0 ) return -1;
	
	// Modified binary search to find closest relevant data point
	int step = size / 2;
	int pos = int(step - .5);
	while(step > 0) {
		int thisStep = step;
		step /= 2;
		QDateTime check = data[pos].first;
		if( check > dt )
			thisStep *= -1;
		if( pos + thisStep >= 0 && pos + thisStep < size ) {
			QDateTime move = data[pos+thisStep].first;
			if( Interval(dt,move).abs() < Interval(dt,check).abs() )
				pos += thisStep;
		}
	}
	LOG_5( dt.toString() + " closest point is " + QString::number(pos) );
	return pos;
}

double GraphiteGetResult::getAverage( const QDateTime & start, const QDateTime & end )
{
	double ret = numeric_limits<double>::quiet_NaN();
	if( data.isEmpty() )
		return ret;
	
	int startI = findClosestDataPoint(start);
	int endI = findClosestDataPoint(end);
	double avg = 0.0;
	int contrib = 0;
	for( int i = startI; i <= endI; i++ ) {
		double d = data[i].second;
		// If not NaN
		if( !(d != d) ) {
			avg += d;
			contrib += 1;
		}
	}
	if( contrib > 0 )
		return avg / contrib;
	return numeric_limits<double>::quiet_NaN();
}


GraphiteGetResult GraphiteGetResult::adjustToInterval( const Interval & intv, const QDateTime & start, int count )
{
	GraphiteGetResult ret;
	ret.interval = intv;
	ret.path = path;
	
	// Do nothing if we have no data or if the interval is negative or 0
	if( data.isEmpty() || intv <= Interval() ) {
		LOG_5( "Returning empty GraphtieGetResult because of negative interval" );
		return ret;
	}
	
	// Fill in default start
	QDateTime cur = start;
	if( !cur.isValid() ) {
		cur = (intv / 2.0).adjust((interval / -2.0).adjust(data[0].first));
	}
	
	if( count < 0 ) {
		QPair<QDateTime,double> lastEntry = data[data.size()-1];
		Interval resultSpan = Interval(data[0].first,lastEntry.first) + interval;
		count =  ceil(resultSpan / intv);
		//LOG_1( QString("data start to end interval: %1, requested interval %2, Computed count: %3").arg(resultSpan.toDisplayString()).arg(intv.toDisplayString()).arg(count) );
	}
	
	while( count-- > 0 ) {
		QDateTime curStart = (intv / -2.0).adjust(cur);
		QDateTime curEnd = (intv / 2.0).adjust(cur);
		//LOG_1( QString( "Getting data from %1 to %2 to compute %3 with interval %4" ).arg( curStart.toString() ).arg( curEnd.toString() ).arg( cur.toString() ).arg( intv.toDisplayString() ) );
		ret.data.append( qMakePair<QDateTime,double>( cur, getAverage( curStart, curEnd ) ) );
		cur = intv.adjust(cur);
	}
	
	return ret;
}

GraphiteDataSource::GraphiteDataSource( QObject * parent )
: GraphiteSource(parent)
{
}

GraphiteDataSource::~GraphiteDataSource()
{
}

void GraphiteDataSource::getData( const QString & path, const QDateTime & start, const QDateTime & end )
{
	GraphiteDesc desc( QStringList() += path, QSize(), GraphiteDesc::None, start, end );
	QUrl url = desc.buildUrl( mGraphiteServer, mGraphitePort );
	url.addQueryItem( "rawData", "true" );
	this->start( QNetworkRequest( url ) );
}


/*
Graphite can return numerical data in a CSV format by simply adding a "rawData=true" url parameter to any graphite image url. The output format is as follows:
target1, startTime, endTime, step | value1, value2, ..., valueN
target2, startTime, endTime, step | value1, value2, ..., valueN
...

Each line corresponds to a graph element. Everything before the "|" on a given line is header information, everything after is numerical values. The header describes the name of the target, the start and end times (in unix epoch time) of the retrieved interval, and the step is the number of seconds between datapoints. So the timestamp of value1 is startTime, the timestamp of value2 is (startTime+step), value3 is (startTime+step+step), etc?
Note that non-existent or null values in a series are represented by the string "None".
*/

void GraphiteDataSource::networkRequestFinished()
{
	if( !mCurrentRequest ) return;
	foreach( QByteArray headerName, mCurrentRequest->rawHeaderList() ) {
		LOG_5( QString::fromLatin1(headerName) + ": " + QString::fromLatin1(mCurrentRequest->rawHeader(headerName)) );
	}
	QTextStream ts( mCurrentRequest );
	while( !ts.atEnd() ) {
		GraphiteGetResult ret;
		QString line = ts.readLine();
		int headerEnd = line.indexOf( '|' );
		if( !headerEnd ) {
			LOG_TRACE
			continue;
		}
		QStringList headers = line.left( headerEnd ).split(",");
		if( headers.size() != 4 ) {
			LOG_TRACE
			continue;
		}
		ret.path = headers[0];
		QDateTime dt = QDateTime::fromTime_t( headers[1].toInt() );
		ret.interval = Interval(headers[3].toInt());
		LOG_5( QString("Parsed entry header: path=%1, datetime=%2, interval=%3").arg(ret.path).arg(dt.toString()).arg(ret.interval.toString()) );
		int pos = headerEnd + 1;
		while( pos <= line.size() ) {
			int entryEnd = line.indexOf( ",", pos );
			if( entryEnd == -1 )
				entryEnd = line.size();
			QString entry = line.mid(pos,entryEnd-pos);
			bool valid = true;
			double data = entry.toDouble(&valid);
			if( !valid && entry == "None" )
				data = numeric_limits<double>::quiet_NaN();
			ret.data.append( qMakePair<QDateTime,double>( dt, data ) );
			dt = ret.interval.adjust(dt);
			// Skip the comma
			pos = entryEnd + 1;
		}
		emit getDataResult( ret );
	}
	mCurrentRequest->deleteLater();
	mCurrentRequest = 0;
}

