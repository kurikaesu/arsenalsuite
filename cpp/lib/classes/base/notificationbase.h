
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

#ifdef HEADER_FILES
#include "notificationdestination.h"
class NotificationEvent;

#endif

#ifdef CLASS_FUNCTIONS

/// Looks up the NotificationEvent record matching event and fkeynotificationcomponent that points to matching component
/// Creates them as needed if create=true
static NotificationEvent findEvent( const QString & component, const QString & event, bool create = true );

static Notification create( const QString & component, const QString & event, const QString & subject, const QString & message = QString(), const QString & brief = QString() );

/// Convenience function for accessing notificationEvent().notificationComponent().name()
QString component() const;
/// Convenience function for accessing notificationEvent().name()
QString event() const;

NotificationDestination sendTo( const User & user, const QString & method = QString() );
NotificationDestination sendTo( const QString & address, const QString & method );

/// Removes matching NotificationDestination records only if they have not yet been delivered
int unsendTo( const User & user );
int unsendTo( const QString & address );

/// Creates a new notification record and updates this with an update text and fkey pointing to the new one(updatedNotification())
/// destinations are not copied so they should be added the same as creating a new event from scratch
/// If subject, message, or brief is empty, they are copied from this record
Notification update( const QString & updateText = QString(), const QString & subject = QString(), const QString & message = QString(), const QString & brief = QString() );

#endif

