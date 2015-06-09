/*
 *
 * Copyright 2006 Blur Studio Inc.
 *
 * This file is part of the Resin software package.
 *
 * Resin is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Resin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Resin; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef COMMIT_CODE

#include "notification.h"
#include "notificationcomponent.h"
#include "notificationevent.h"
#include "notificationmethod.h"
#include "user.h"

NotificationEvent Notification::findEvent( const QString & component, const QString & event, bool create )
{
	NotificationComponent nc = NotificationComponent::recordByName( component );
	if( !nc.isRecord() ) {
		if( !create ) return NotificationEvent();
		nc.setName( component );
		nc.commit();
	}
	NotificationEvent ne = NotificationEvent::recordByComponentAndEvent( nc, event );
	if( !ne.isRecord() ) {
		if( !create ) return NotificationEvent();
		ne.setName( event );
		ne.setNotificationComponent( nc );
		ne.commit();
	}
	return ne;
}

QString Notification::component() const
{
	return notificationEvent().notificationComponent().name();
}

QString Notification::event() const
{
	return notificationEvent().name();
}

Notification Notification::create( const QString & component, const QString & event, const QString & subject, const QString & message, const QString & brief )
{
	Notification n;
	n.setNotificationEvent( findEvent( component, event ) );
	// Backwards compat
	n.setEventCompat( event );
	n.setComponentCompat( component );
	// End backwards compat
	n.setSubject( subject );
	n.setMessage( message );
	n.setBrief( brief );
	n.commit();
	return n;
}

NotificationDestination Notification::sendTo( const User & user, const QString & method )
{
	NotificationDestination nd;
	nd.setUser( user );
	nd.setNotificationMethod( NotificationMethod::recordByName( method ) );
	nd.setNotification( *this );
	nd.commit();
	return nd;
}

NotificationDestination Notification::sendTo( const QString & address, const QString & method )
{
	NotificationDestination nd;
	nd.setNotification( *this );
	nd.setNotificationMethod( NotificationMethod::recordByName( method ) );
	nd.setDestination( address );
	nd.commit();
	return nd;
}

int Notification::unsendTo( const User & user )
{
	int removed = 0;
	NotificationDestinationList dests = notificationDestinations();
	foreach( NotificationDestination dest, dests ) {
		if( dest.user() == user && dest.delivered().isNull() ) {
			dest.remove();
			removed++;
		}
	}
	return removed;
}

int Notification::unsendTo( const QString & address )
{
	int removed = 0;
	NotificationDestinationList dests = notificationDestinations();
	foreach( NotificationDestination dest, dests ) {
		if( dest.destination() == address && dest.delivered().isNull() ) {
			dest.remove();
			removed++;
		}
	}
	return removed;
}

Notification Notification::update( const QString & updateText, const QString & _subject, const QString & _message, const QString & _brief )
{
	Notification n = create( notificationEvent().notificationComponent().name(), notificationEvent().name(), _subject.isEmpty() ? subject() : _subject, _message.isEmpty() ? message() : _message, _brief.isEmpty() ? brief() : _brief );
	setUpdatedNotification( n );
	setUpdateText( updateText );
	commit();
	return n;
}

#endif // CLASS_FUNCTIONS

