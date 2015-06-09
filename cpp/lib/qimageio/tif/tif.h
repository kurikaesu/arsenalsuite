/**
* QImageIO Routines to read/write TIFF images.
* copyright (c) 1998 Sirtaj Singh Kang <taj@kde.org>
*
* This library is distributed under the conditions of the GNU LGPL.
*/

#ifndef TIF_H
#define TIF_H

#include <QtGui/QImageIOPlugin>

class TIFHandler : public QImageIOHandler
{
public:
    TIFHandler();

    bool canRead() const;
    bool read(QImage *image);
    bool write(const QImage &image);

    QByteArray name() const;

    static bool canRead(QIODevice *device);
};

#endif

