
import blur.quickinit

# static Notification create( const QString & component, const QString & event, const QString & subject, const QString & message = QString() );
note = blur.quickinit.Notification.create( 'Point Cache', 'Updated', 'Point Cache Updated' )

# Send by specifying an actual user object
# Leave method black for default delivery method
# NotificationDestination sendTo( const User & user, const QString & method = QString() );
note.sendTo( blur.quickinit.User.recordByUserName( 'newellm' ), 'Email' )

# Send by giving address, must corrospond with method
# NotificationDestination sendTo( const QString & address, const QString & method );
note.sendTo( 'eric@blur.com', 'Email' )

