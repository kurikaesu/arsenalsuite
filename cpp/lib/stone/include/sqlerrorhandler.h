
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
 * $Id$
 */

#ifndef SQL_ERROR_HANDLER_H
#define SQL_ERROR_HANDLER_H

#include <qstring.h>

#include "blurqt.h"

/**
 * Default implementation looks up [Database].SqlErrorEmailList and sends
 * and email containing the sql error, host, application name, and current
 * user.  Sends nothing if SqlErrorEmailList is empty.  If SqlErrorEmailList
 * key is missing, sends to newellm@blur.com.
 */
class STONE_EXPORT SqlErrorHandler
{
public:
	virtual ~SqlErrorHandler(){}
	virtual void handleError(const QString & error);

	static void setInstance(SqlErrorHandler *);
	static SqlErrorHandler * instance();
};

#endif // SQL_ERROR_HANDLER_H

