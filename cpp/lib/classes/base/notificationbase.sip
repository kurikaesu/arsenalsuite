
static MappedNotificationEvent findEvent( const QString & component, const QString & event, bool create = true );

static MappedNotification create( const QString & component, const QString & event, const QString & subject, const QString & message = QString(), const QString & brief = QString() );

QString component() const;
QString event() const;

MappedNotificationDestination sendTo( const User & user, const QString & method = QString() );
MappedNotificationDestination sendTo( const QString & address, const QString & method );

int unsendTo( const User & user );
int unsendTo( const QString & address );

MappedNotification update( const QString & updateText = QString(), const QString & subject = QString(), const QString & message = QString(), const QString & brief = QString() );

