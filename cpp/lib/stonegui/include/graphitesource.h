
#ifndef GRAPHITE_SOURCE_H
#define GRAPHITE_SOURCE_H

#include <limits.h>

#include <qobject.h>
#include <qsize.h>
#include <qstringlist.h>
#include <qdatetime.h>
#include <qnetworkreply.h>

#include "interval.h"

#include "stonegui.h"

class QImage;
class QNetworkAccessManager;
class QNetworkReply;
class QStringList;


class STONEGUI_EXPORT GraphiteDesc
{
public:
	enum AreaMode {
		None,
		First,
		All,
		Stacked
	};
	static GraphiteDesc::AreaMode areaModeFromString( const QString & areaModeStr );
	static QString areaModeToString( GraphiteDesc::AreaMode areaMode );

	GraphiteDesc( QStringList sources = QStringList(), QSize size = QSize(640,480), AreaMode areaMode = None, const QDateTime & start = QDateTime(), const QDateTime & end = QDateTime(), int minValue = INT_MAX, int maxValue = INT_MAX );
	
	~GraphiteDesc();
		
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
	Interval relativeStart() const;
	Interval relativeEnd() const;
	
	void setStart( const QDateTime & start );
	void setEnd( const QDateTime & end );
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
	void appendExtra( QPair<QString,QString> extra );
	
	// Returns a time series of graphite descriptions based on the current settings
	// Each graph will cover the same amount of time as this one, and will be adjacent
	// to the one before it, starting with this one.
	QList<GraphiteDesc> generateTimeSeries( int count ) const;
	
protected:
	QStringList mSources;
	QSize mSize;
	AreaMode mAreaMode;
	QDateTime mStart, mEnd;
	Interval mRelativeStart, mRelativeEnd;
	int mMinValue, mMaxValue;
	QList<QPair<QString,QString> > mExtra;
};

class STONEGUI_EXPORT GraphiteSource : public QObject
{
Q_OBJECT
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
	
protected slots:
	virtual void networkRequestFinished();
	void networkRequestError( QNetworkReply::NetworkError );

protected:
	void _error( const QString & errorMessage );
	
	static int sNetworkAccessManagerRefCount;
	static QNetworkAccessManager * sNetworkAccessManager;
	QNetworkReply * mCurrentRequest;
	QString mGraphiteServer;
	qint64 mGraphitePort;
};

class STONEGUI_EXPORT GraphiteImageSource : public GraphiteSource
{
Q_OBJECT
public:
	GraphiteImageSource( QObject * parent = 0 );
	~GraphiteImageSource();

signals:
	void getImageProgress( qint64 recieved, qint64 total );
	void getImageFinished( const QImage & image );

public slots:
	void getImage( const GraphiteDesc & );
	
protected:
	void networkRequestFinished();
};


class STONEGUI_EXPORT GraphiteGetResult
{
public:
	QString path;
	Interval interval;
	QList<QPair<QDateTime,double> > data;

	int findClosestDataPoint( const QDateTime & dt );
	double getAverage( const QDateTime & start, const QDateTime & end );
	GraphiteGetResult adjustToInterval( const Interval &, const QDateTime & start = QDateTime(), int count = -1 );
};

class STONEGUI_EXPORT GraphiteDataSource : public GraphiteSource
{
Q_OBJECT
public:
	GraphiteDataSource( QObject * parent = 0 );
	~GraphiteDataSource();

signals:
	void getDataResult( const GraphiteGetResult & );

public slots:
	void getData( const QString & path, const QDateTime & start, const QDateTime & end );

protected:
	void networkRequestFinished();
};

#endif // GRAPHITE_SOURCE_H
