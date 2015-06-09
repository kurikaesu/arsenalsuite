
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of libstone.
 *
 * libstone is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libstone is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libstone; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id: actions.h 5411 2007-12-18 01:03:08Z brobison $
 */

#ifndef ACTIONS_H
#define ACTIONS_H

#include "stonegui.h"

class QAction;
class QObject;

/// Convenience function for creating a separator action
STONEGUI_EXPORT QAction * separatorAction( QObject * parent );

/// Convenience function for creating a checkable/checked action, with a receiver
STONEGUI_EXPORT QAction * checkableAction( const QString & text, QObject * parent, bool checked, QObject * recv = 0, const char * member = 0, const QIcon & icon = QIcon() );

/// Convenience function for creating a QAction with the triggered signal connected to another object's slot
STONEGUI_EXPORT QAction * connectedAction( const QString & text, QObject * parent, QObject * recv, const char * member, const QIcon & icon = QIcon() );

#endif // ACTIONS_H

