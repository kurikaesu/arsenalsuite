// kimgio module for SGI images
//
// Copyright (C) 2004  Melchior FRANZ  <mfranz@kde.org>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the Lesser GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.


/* this code supports:
 * reading:
 *     everything, except images with 1 dimension or images with
 *     mapmode != NORMAL (e.g. dithered); Images with 16 bit
 *     precision or more than 4 layers are stripped down.
 * writing:
 *     Run Length Encoded (RLE) or Verbatim (uncompressed)
 *     (whichever is smaller)
 *
 * Please report if you come across rgb/rgba/sgi/bw files that aren't
 * recognized. Also report applications that can't deal with images
 * saved by this filter.
 */


#include "sgi.h"
#include <qimage.h>

SGIImage::SGIImage(QIODevice *io) :
    m_starttab(0),
    m_lengthtab(0)
{
    m_dev = io;
    m_stream.setDevice(m_dev);
}


SGIImage::~SGIImage()
{
    delete[] m_starttab;
    delete[] m_lengthtab;
}




bool SGIImage::getRow(uchar *dest)
{
    int n, i;
    if (!m_rle) {
        for (i = 0; i < m_xsize; i++) {
            if (m_pos >= m_data.end())
                return false;
            dest[i] = uchar(*m_pos);
            m_pos += m_bpc;
        }
        return true;
    }

    for (i = 0; i < m_xsize;) {
        if (m_bpc == 2)
            m_pos++;
        n = *m_pos & 0x7f;
        if (!n)
            break;

        if (*m_pos++ & 0x80) {
            for (; i < m_xsize && n--; i++) {
                *dest++ = *m_pos;
                m_pos += m_bpc;
            }
        } else {
            for (; i < m_xsize && n--; i++)
                *dest++ = *m_pos;

            m_pos += m_bpc;
        }
    }
    return i == m_xsize;
}


bool SGIImage::readData(QImage& img)
{
    QRgb *c;
    quint32 *start = m_starttab;
    QByteArray lguard(m_xsize, 0);
    uchar *line = (uchar *)lguard.data();
    unsigned x, y;

    if (!m_rle)
        m_pos = m_data.begin();

    for (y = 0; y < m_ysize; y++) {
        if (m_rle)
            m_pos = m_data.begin() + *start++;
        if (!getRow(line))
            return false;
        c = (QRgb *)img.scanLine(m_ysize - y - 1);
        for (x = 0; x < m_xsize; x++, c++)
            *c = qRgb(line[x], line[x], line[x]);
    }

    if (m_zsize == 1)
        return true;

    if (m_zsize != 2) {
        for (y = 0; y < m_ysize; y++) {
            if (m_rle)
                m_pos = m_data.begin() + *start++;
            if (!getRow(line))
                return false;
            c = (QRgb *)img.scanLine(m_ysize - y - 1);
            for (x = 0; x < m_xsize; x++, c++)
                *c = qRgb(qRed(*c), line[x], line[x]);
        }

        for (y = 0; y < m_ysize; y++) {
            if (m_rle)
                m_pos = m_data.begin() + *start++;
            if (!getRow(line))
                return false;
            c = (QRgb *)img.scanLine(m_ysize - y - 1);
            for (x = 0; x < m_xsize; x++, c++)
                *c = qRgb(qRed(*c), qGreen(*c), line[x]);
        }

        if (m_zsize == 3)
            return true;
    }

    for (y = 0; y < m_ysize; y++) {
        if (m_rle)
            m_pos = m_data.begin() + *start++;
        if (!getRow(line))
            return false;
        c = (QRgb *)img.scanLine(m_ysize - y - 1);
        for (x = 0; x < m_xsize; x++, c++)
            *c = qRgba(qRed(*c), qGreen(*c), qBlue(*c), line[x]);
    }

    return true;
}


bool SGIImage::readImage(QImage& img)
{
    qint8 u8;
    qint16 u16;
    qint32 u32;

    // magic
    m_stream >> u16;
    if (u16 != 0x01da)
        return false;

    // verbatim/rle
    m_stream >> m_rle;
    if (m_rle > 1)
        return false;

    // bytes per channel
    m_stream >> m_bpc;
    if (m_bpc == 1 || m_bpc == 2)
        ;
    else
        return false;

    // number of dimensions
    m_stream >> m_dim;
    if (m_dim < 1 || m_dim > 3)
        return false;

    m_stream >> m_xsize >> m_ysize >> m_zsize >> m_pixmin >> m_pixmax >> u32;

    // name
    m_stream.readRawData(m_imagename, 80);
    m_imagename[79] = '\0';

    m_stream >> m_colormap;
    if (m_colormap != NORMAL)
        return false;       // only NORMAL supported

    for (int i = 0; i < 404; i++)
        m_stream >> u8;

    if (m_dim == 1) {
        return false;
    }

    if( m_stream.atEnd())
        return false;

    m_numrows = m_ysize * m_zsize;


    if (m_zsize == 2 || m_zsize == 4)
        img = QImage( m_xsize, m_ysize, QImage::Format_ARGB32 );
	else
	    img = QImage( m_xsize, m_ysize, QImage::Format_RGB32 );

    if (m_rle) {
        uint l;
        m_starttab = new quint32[m_numrows];
        for (l = 0; !m_stream.atEnd() && l < m_numrows; l++) {
            m_stream >> m_starttab[l];
            m_starttab[l] -= 512 + m_numrows * 2 * sizeof(quint32);
        }

        m_lengthtab = new quint32[m_numrows];
        for (l = 0; l < m_numrows; l++)
            m_stream >> m_lengthtab[l];
    }

    m_data = m_dev->readAll();

    // sanity check
    if (m_rle)
        for (uint o = 0; o < m_numrows; o++)
            // don't change to greater-or-equal!
            if (m_starttab[o] + m_lengthtab[o] > (uint)m_data.size()) {
                return false;
            }

    if (!readData(img)) {
        return false;
    }

    return true;
}



void RLEData::write(QDataStream& s)
{
    for (int i = 0; i < size(); i++)
        s << at(i);
}


bool RLEData::operator<(const RLEData& b) const
{
    uchar ac, bc;
    for (int i = 0; i < qMin(size(), b.size()); i++) {
        ac = at(i);
        bc = b[i];
        if (ac != bc)
            return ac < bc;
    }
    return size() < b.size();
}


uint RLEMap::insert(const uchar *d, uint l)
{
    RLEData data = RLEData(d, l, m_offset);
    Iterator it = find(data);
    if (it != end())
        return it.value();

    m_offset += l;
    return QMap<RLEData, uint>::insert(data, m_counter++).value();
}


QVector<const RLEData*> RLEMap::vector()
{
    QVector<const RLEData*> v(size());
    for (Iterator it = begin(); it != end(); ++it)
        v.insert(it.value(), &it.key());

    return v;
}


uchar SGIImage::intensity(uchar c)
{
    if (c < m_pixmin)
        m_pixmin = c;
    if (c > m_pixmax)
        m_pixmax = c;
    return c;
}


uint SGIImage::compact(uchar *d, uchar *s)
{
    uchar *dest = d, *src = s, patt, *t, *end = s + m_xsize;
    int i, n;
    while (src < end) {
        for (n = 0, t = src; t + 2 < end && !(*t == t[1] && *t == t[2]); t++)
            n++;

        while (n) {
            i = n > 126 ? 126 : n;
            n -= i;
            *dest++ = 0x80 | i;
            while (i--)
                *dest++ = *src++;
        }

        if (src == end)
            break;

        patt = *src++;
        for (n = 1; src < end && *src == patt; src++)
            n++;

        while (n) {
            i = n > 126 ? 126 : n;
            n -= i;
            *dest++ = i;
            *dest++ = patt;
        }
    }
    *dest++ = 0;
    return dest - d;
}


bool SGIImage::scanData(const QImage& img)
{
    quint32 *start = m_starttab;
    QByteArray lineguard(m_xsize * 2, 0);
    QByteArray bufguard(m_xsize, 0);
    uchar *line = (uchar *)lineguard.data();
    uchar *buf = (uchar *)bufguard.data();
    const QRgb *c;
    unsigned x, y;
    uint len;

    for (y = 0; y < m_ysize; y++) {
        c = reinterpret_cast<const QRgb *>(img.scanLine(m_ysize - y - 1));
        for (x = 0; x < m_xsize; x++)
            buf[x] = intensity(qRed(*c++));
        len = compact(line, buf);
        *start++ = m_rlemap.insert(line, len);
    }

    if (m_zsize == 1)
        return true;

    if (m_zsize != 2) {
        for (y = 0; y < m_ysize; y++) {
            c = reinterpret_cast<const QRgb *>(img.scanLine(m_ysize - y - 1));
            for (x = 0; x < m_xsize; x++)
                buf[x] = intensity(qGreen(*c++));
            len = compact(line, buf);
            *start++ = m_rlemap.insert(line, len);
        }

        for (y = 0; y < m_ysize; y++) {
            c = reinterpret_cast<const QRgb *>(img.scanLine(m_ysize - y - 1));
            for (x = 0; x < m_xsize; x++)
                buf[x] = intensity(qBlue(*c++));
            len = compact(line, buf);
            *start++ = m_rlemap.insert(line, len);
        }

        if (m_zsize == 3)
            return true;
    }

    for (y = 0; y < m_ysize; y++) {
        c = reinterpret_cast<const QRgb *>(img.scanLine(m_ysize - y - 1));
        for (x = 0; x < m_xsize; x++)
            buf[x] = intensity(qAlpha(*c++));
        len = compact(line, buf);
        *start++ = m_rlemap.insert(line, len);
    }

    return true;
}


void SGIImage::writeHeader()
{
    m_stream << quint16(0x01da);
    m_stream << m_rle << m_bpc << m_dim;
    m_stream << m_xsize << m_ysize << m_zsize;
    m_stream << m_pixmin << m_pixmax;
    m_stream << quint32(0);

    m_stream << m_colormap;
    for (int i = 0; i < 404; i++)
        m_stream << quint8(0);
}


void SGIImage::writeRle()
{
    m_rle = 1;
    writeHeader();
    uint i;

    // write start table
    for (i = 0; i < m_numrows; i++)
        m_stream << quint32(m_rlevector[m_starttab[i]]->offset());

    // write length table
    for (i = 0; i < m_numrows; i++)
        m_stream << quint32(m_rlevector[m_starttab[i]]->size());

    // write data
    for (i = 0; (int)i < m_rlevector.size(); i++)
        const_cast<RLEData*>(m_rlevector[i])->write(m_stream);
}


void SGIImage::writeVerbatim(const QImage& img)
{
    m_rle = 0;
    writeHeader();

    const QRgb *c;
    unsigned x, y;

    for (y = 0; y < m_ysize; y++) {
        c = reinterpret_cast<const QRgb *>(img.scanLine(m_ysize - y - 1));
        for (x = 0; x < m_xsize; x++)
            m_stream << quint8(qRed(*c++));
    }

    if (m_zsize == 1)
        return;

    if (m_zsize != 2) {
        for (y = 0; y < m_ysize; y++) {
            c = reinterpret_cast<const QRgb *>(img.scanLine(m_ysize - y - 1));
            for (x = 0; x < m_xsize; x++)
                m_stream << quint8(qGreen(*c++));
        }

        for (y = 0; y < m_ysize; y++) {
            c = reinterpret_cast<const QRgb *>(img.scanLine(m_ysize - y - 1));
            for (x = 0; x < m_xsize; x++)
                m_stream << quint8(qBlue(*c++));
        }

        if (m_zsize == 3)
            return;
    }

    for (y = 0; y < m_ysize; y++) {
        c = reinterpret_cast<const QRgb *>(img.scanLine(m_ysize - y - 1));
        for (x = 0; x < m_xsize; x++)
            m_stream << quint8(qAlpha(*c++));
    }
}


bool SGIImage::writeImage(const QImage& image)
{
    QImage img = image;
    if (img.allGray())
        m_dim = 2, m_zsize = 1;
    else
        m_dim = 3, m_zsize = 3;

    if (img.format() == QImage::Format_ARGB32)
        m_dim = 3, m_zsize++;

    img = img.convertToFormat(QImage::Format_RGB32);
    if (img.isNull()) {
        return false;
    }

    m_bpc = 1;
    m_xsize = img.width();
    m_ysize = img.height();
    m_pixmin = ~0;
    m_pixmax = 0;
    m_colormap = NORMAL;

    m_numrows = m_ysize * m_zsize;

    m_starttab = new quint32[m_numrows];
    m_rlemap.setBaseOffset(512 + m_numrows * 2 * sizeof(quint32));

    if (!scanData(img)) {
        return false;
    }

    m_rlevector = m_rlemap.vector();

    long verbatim_size = m_numrows * m_xsize;
    long rle_size = m_numrows * 2 * sizeof(quint32);
    for (int i = 0; i < m_rlevector.size(); i++)
        rle_size += m_rlevector[i]->size();

    if (verbatim_size <= rle_size)
        writeVerbatim(img);
    else
        writeRle();
    return true;
}


RGBHandler::RGBHandler()
{
}

bool RGBHandler::canRead() const
{
    return canRead(device());
}

bool RGBHandler::read(QImage *outImage)
{
    SGIImage sgi(device());

    if (!sgi.readImage(*outImage)) {
        return false;
    }

    return true;
}

bool RGBHandler::write(const QImage &image)
{
    SGIImage sgi(device());

    if (!sgi.writeImage(image))
        return false;

    return true;
}

QByteArray RGBHandler::name() const
{
    return "rgb";
}

bool RGBHandler::canRead(QIODevice *device)
{
    if (!device) {
        qWarning("RGBHandler::canRead() called with no device");
        return false;
    }

    qint64 oldPos = device->pos();
    QByteArray head = device->readLine(64);
    int readBytes = head.size();

    if (device->isSequential()) {
        while (readBytes > 0)
            device->ungetChar(head[readBytes-- - 1]);
    } else {
        device->seek(oldPos);
    }

    const QRegExp regexp("^\x01\xda\x01[\x01\x02]");
    QString data(head);

    return data.contains(regexp);
}


class RGBPlugin : public QImageIOPlugin
{
public:
    QStringList keys() const;
    Capabilities capabilities(QIODevice *device, const QByteArray &format) const;
    QImageIOHandler *create(QIODevice *device, const QByteArray &format = QByteArray()) const;
};

QStringList RGBPlugin::keys() const
{
    return QStringList() << "rgb"<<"RGB"<<"rgba"<<"RGBA"
                         <<"bw"<<"BW"<<"sgi"<<"SGI";
}

QImageIOPlugin::Capabilities RGBPlugin::capabilities(QIODevice *device, const QByteArray &format) const
{
    if (format == "rgb" || format == "rgb" || format == "RGB" ||
        format ==  "rgba" || format == "RGBA" || format ==  "bw" ||
        format == "BW" || format == "sgi" || format == "SGI")
        return Capabilities(CanRead | CanWrite);
    if (!format.isEmpty())
        return 0;
    if (!device->isOpen())
        return 0;

    Capabilities cap;
    if (device->isReadable() && RGBHandler::canRead(device))
        cap |= CanRead;
    if (device->isWritable())
        cap |= CanWrite;
    return cap;
}

QImageIOHandler *RGBPlugin::create(QIODevice *device, const QByteArray &format) const
{
    QImageIOHandler *handler = new RGBHandler;
    handler->setDevice(device);
    handler->setFormat(format);
    return handler;
}

Q_EXPORT_STATIC_PLUGIN(RGBPlugin)
Q_EXPORT_PLUGIN2(rgb, RGBPlugin)
