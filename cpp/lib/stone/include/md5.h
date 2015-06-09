/***************************************************************************
 *                                                                         *
 *   copyright (C) 2004 by Michael Buesch                                  *
 *   email: mbuesch@freenet.de                                             *
 *                                                                         *
 *   md5.c - MD5 Message-Digest Algorithm                                  *
 *   Copyright (C) 1995,1996,1998,1999,2001,2002,                          *
 *                 2003  Free Software Foundation, Inc.                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 *                                                                         *
 ***************************************************************************/

#ifndef MD5_H
#define MD5_H

#include "md5_globalstuff.h"

#include "blurqt.h"

//#include <stdint.h>
#include <string>
using std::string;

#define MD5_HASHLEN_BYTE	(128 / 8)

class STONE_EXPORT Md5
{
	struct MD5_CONTEXT
	{
		quint32 A, B, C, D;	/* chaining variables */
		quint32 nblocks;
		byte buf[64];
		int count;
	};

public:
	Md5() {}
	virtual ~Md5() {}
	static bool selfTest();

	//string calcMd5(const string &buf);
	QString calcMd5(const QString &filename);
	//string calcMd5(const string &filename, int *commSocket);

protected:
	void md5_init(MD5_CONTEXT *ctx);
	void transform(MD5_CONTEXT *ctx, const byte *data);
	void md5_write(MD5_CONTEXT *hd, const byte *inbuf, size_t inlen);
	byte * md5_final(MD5_CONTEXT *hd);
	void burn_stack(int bytes);
};

#endif
