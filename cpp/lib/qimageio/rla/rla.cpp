
#include <stdio.h>

#include <qimage.h>

#include "rla.h"

#ifdef __GNUC__
#define PACKED __attribute__ ((__packed__))
#else
#define PACKED
#endif

struct PACKED RLARect {
	qint16 left;
	qint16 right;
	qint16 bottom;
	qint16 top;
};

const qint16 RLA_MAGIC = 0xFFFE;
const qint16 RPF_MAGIC  = 0xFFFD;
const int RLA_Y_PAGE_SIZE = 32;

struct PACKED RLAHeader {
	RLARect		window;
	RLARect		active_window;
	qint16 		frame;
	qint16 		storage_type;
	qint16 		num_chan;
	qint16 		num_matte;
	qint16 		num_aux;
	qint16		revision;
	qint8		gamma[16];
	qint8   	red_pri[24];
	qint8		green_pri[24];
	qint8		blue_pri[24];
	qint8		white_pt[24];
	qint32		job_num;
	qint8		name[128];
	qint8		desc[128];
	qint8		program[64];
	qint8		machine[32];
	qint8		user[32];
	qint8		date[20];
	qint8		aspect[24];
	qint8		aspect_ratio[8];
	qint8		chan[32];
	qint16		field;
	qint8		time[12];
	qint8		filter[32];
	qint16		chan_bits;
	qint16		matte_type;
	qint16		matte_bits;
	qint16		aux_type;
	qint16		aux_bits;
	qint8		aux[32];
	qint8		space[36];
	qint32		next;
};

template<class T> T lswap(T l) {
	return ((l >> 24) & 0x000000ff) |
		((l >>  8) & 0x0000ff00) |
		((l <<  8) & 0x00ff0000) |
		((l << 24) & 0xff000000);
}

template<class T> T sswap(T s)
{
	return ((s >> 8) & 0x00ff) | ((s << 8) & 0xff00);
}

#define LSW(x) { x = lswap(x); }
#define SSW(x) { x = sswap(x); }

class RLAReader
{
public:
	RLAReader()
	: device(0)
	, currentFileOffset( 0 )
	{}
	
	bool init(QIODevice * _device) {
		device = _device;
		if( !device ) return false;
		return readHeader();
	}

	bool readHeader() {
		if( read( (char*)&header, sizeof(header) ) < 0 ) {
			fprintf( stderr, "Unable to read full rla header" );
			return false;
		}
		
		SSW(header.window.left);
		SSW(header.window.right);
		SSW(header.window.top);
		SSW(header.window.bottom);
		SSW(header.active_window.left);
		SSW(header.active_window.right);
		SSW(header.active_window.top);
		SSW(header.active_window.bottom);
		SSW(header.frame);
		SSW(header.storage_type);
		SSW(header.num_chan);
		SSW(header.num_matte);
		SSW(header.num_aux);
		SSW(header.revision);
		LSW(header.job_num);
		SSW(header.field);
		SSW(header.chan_bits);
		SSW(header.matte_type);
		SSW(header.matte_bits);
		SSW(header.aux_bits);
		LSW(header.next);

		if( (header.revision != RLA_MAGIC) && (header.revision != RPF_MAGIC) ) {
			fprintf( stderr, "Header doesn't match RLA or RPF magic\n" );
			return false;
		}
		
		width = header.active_window.right - header.active_window.left + 1;
		height = header.active_window.top - header.active_window.bottom + 1;
		hasAlpha = header.num_matte > 0;
		fprintf( stderr, "Image dimensions %i x %i with %salpha\n", width, height, hasAlpha ? "" : "no " );
		return true;
	}

	QImage read() {
		QImage ret( width, height, hasAlpha ? QImage::Format_ARGB32 : QImage::Format_RGB32 );
		quint32 * offset_table = new quint32[height];
		
		// Read and convert offset table
		if( read( (char*)offset_table, sizeof(qint32) * height ) < 0 ) {
			fprintf( stderr, "Unable to read the offset table, file must be incomplete\n" );
			return QImage();
		}

		// Convert to little endian
		for( int i=0; i<height; i++ )
			LSW(offset_table[i]);
		
		// Read image data
		for( int y=height-1; y>=0; y-- )
			if( !readScanLine(offset_table[y], ret.scanLine(height-y-1)) ) {
				fprintf( stderr, "Error while reading scanline %i\n", y );
				return QImage();
			}
	
		return ret;
	}

	bool readScanLine( int offset, uchar * dest_scanline )
	{
		// Byte          RLA     |   QImage
		// 0             R             B
		// 1             G             G
		// 2             B             R
		// 3             A             A
		int readCount = hasAlpha ? 4 : 3;
		for( int rc = 0; rc < readCount; rc++ ) {
			uchar * scanStart = dest_scanline + (2-rc);
			if( hasAlpha && rc == 3 ) scanStart = dest_scanline + 3;
			
			offset = readScanChunk( offset, scanStart, (hasAlpha && rc==3) ? header.matte_bits : header.chan_bits );
			if( offset < 0 )
				return false;
		}
		return true;
	}

	int readScanChunk( int offset, uchar * scanline, int bits ) 
	{
		if( !advanceTo(offset) )
			return -1;

		qint16 length = 0;

		// Read chunk length
		if( read((char*)&length,sizeof(length)) < 0 )
			return -1;
		
		// Swap
		SSW(length);

		if( bits == 4 ) {
			// Double check validity
			if( length % 4 != 0 ) {
				fprintf( stderr, "Length of float scanline chunk isn't divisable by 4!\n" );
			}
			if( length / 4 != width ) {
				fprintf( stderr, "Length of float scanline chunk doesn't match image width\n" );
			}
		}

		// Reserve space
		readBuffer.reserve(length);

		// Read chunk
		if( read(readBuffer.data(), length) < 0 )
			return -1;

		if( bits == 32 ) {
			// Swap to little endian and scale to 8-bit value for QImage
			quint32 * buffer = (quint32*)readBuffer.data();
			for( int x = 0; x < width; ++x ) {
				quint32 l = lswap(*(buffer + x));
				float c = *(float*)(&l);
				*scanline = quint8(c * 255);
				// Stride of 4 for all supported qimage formats
				scanline += 4;
			}
		} else {
			// 16 bit channels are stored as an array of high bytes followed by an array of low bytes,
			// we can ignore the low byte array when converting to 8-bit per channel image
			decode();
			uchar * buffer = (uchar*)decoded.data();
			for( int x = 0; x < width; ++x ) {
				*scanline = buffer[x];
				scanline += 4;
			}
		}
		return offset + length + 2;
	}

	void decode()
	{
		int count, x = width;
		int useX  = 0;
		decoded.reserve(width);

		uchar * out = (uchar*)decoded.data();
		uchar * input = (uchar*)readBuffer.data();
		while (x > 0) {
			count = *(signed char *)input++;
			if (count >= 0) {
				// Run length encoded
				// Repeat pixel value (count + 1) times.
				while (count-- >= 0) {
					if (useX < width) {
						*out = *input;
						out += 1;
					}
					--x;
					useX++;
				}
				++input;
			} else {
				// Copy (-count) unencoded values.
				for (count = -count; count > 0; --count) {
					if (useX < width) {
						*out = *input;
						out += 1;
					}
					input++;
					--x;
					useX++;
				}
			}
		}
	}

	bool advanceTo(int _offset) {
		if( _offset < currentFileOffset ) {
			fprintf( stderr, "Seeking backwards isn't supported for non-sequential QIODevice's: At %i, Seeking %i.\n", currentFileOffset, _offset );
		}
		if( device->isSequential() ) {
			int advance = _offset - currentFileOffset;
			readBuffer.reserve( qMin(advance,1024) );
			while( advance > 0 && read( readBuffer.data(), qMin(advance,1024) ) > 0 ) advance -= qMin(advance,1024);
			if( advance > 0 ) {
				fprintf( stderr, "RLAReader::advanceTo: Error reading from device\n" );
				return false;
			}
		} else {
			device->seek(_offset);
		}
		currentFileOffset = _offset;
		return true;
	}

	int read( char * buffer, qint64 maxLen )
	{
		int read = 0, left = maxLen;
		while( left > 0 && (read = device->read( buffer, maxLen ) ) > 0 ) {
			left -= read;
			currentFileOffset += read;
			read = 0;
		}
		if( left > 0 ) {
			fprintf( stderr, "RLAReader::read: Error reading from device\n" );
			return -1;
		}
		return maxLen;
	}

	QIODevice * device;
	RLAHeader header;
	int width, height;
	bool hasAlpha;

	QByteArray readBuffer, decoded;
	
	int currentFileOffset;
};

RLAHandler::RLAHandler()
{
}

bool RLAHandler::canRead() const
{
	return canRead(device());
}

bool RLAHandler::read(QImage *image)
{
	RLAReader reader;
	if( !reader.init(device()) )
		return false;

	*image = reader.read();
	return true;
}

bool RLAHandler::write(const QImage &)
{
	return false;
}

QByteArray RLAHandler::name() const
{
	return "rla";
}

bool RLAHandler::canRead(QIODevice *device)
{
    if (!device) {
        fprintf( stderr, "TGAHandler::canRead() called with no device\n" );
        return false;
    }

	// Read enough of the header to get the magic
	qint64 oldPos = device->pos();
	int part_header_size = offsetof(RLAHeader,revision)+sizeof(quint16);
	QByteArray part_header = device->read(part_header_size);
	int readBytes = part_header.size();

	if( readBytes != part_header_size ) {
		fprintf( stderr, "RLAHandler::canRead(): Unable to read enough bytes to determine file magic\n" );
		return false;
	}

	// Restore the device position
	if (device->isSequential()) {
		while (readBytes > 0)
			device->ungetChar(part_header[readBytes-- - 1]);
	} else {
		device->seek(oldPos);
	}
	short magic = *(short*)(part_header.data() + offsetof(RLAHeader,revision));
	magic = sswap(magic);
	bool ret = (magic == RLA_MAGIC) || (magic == RPF_MAGIC);
	if( !ret )
		fprintf( stderr, "RLAHandler::canRead() image header does not match rla or rpf magic\n" );
	return ret;
	
}


class RLAPlugin : public QImageIOPlugin
{
public:
    QStringList keys() const;
    Capabilities capabilities(QIODevice *device, const QByteArray &format) const;
    QImageIOHandler *create(QIODevice *device, const QByteArray &format = QByteArray()) const;
};

QStringList RLAPlugin::keys() const
{
    return QStringList() << "RLA" << "rla" << "RPF" << "rpf";
}

QImageIOPlugin::Capabilities RLAPlugin::capabilities(QIODevice *device, const QByteArray &format) const
{
    if (format == "RLA" || format == "rla" || format == "RPF" || format == "rpf" )
        return Capabilities(CanRead); // | CanWrite);
    if (!format.isEmpty())
        return 0;
    if (!device->isOpen())
        return 0;

    Capabilities cap;
    if (device->isReadable() && RLAHandler::canRead(device))
        cap |= CanRead;
//    if (device->isWritable())
//        cap |= CanWrite;
    return cap;
}

QImageIOHandler *RLAPlugin::create(QIODevice *device, const QByteArray &format) const
{
    QImageIOHandler *handler = new RLAHandler;
    handler->setDevice(device);
    handler->setFormat(format);
    return handler;
}

Q_EXPORT_STATIC_PLUGIN(RLAPlugin)
Q_EXPORT_PLUGIN2(RLA, RLAPlugin)


