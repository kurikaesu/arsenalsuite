
#ifndef QIMGIO_RLA_H
#define QIMGIO_RLA_H

#include <QImageIOHandler>

class RLAHandler : public QImageIOHandler
{
public:
	RLAHandler();

	bool canRead() const;
	bool read(QImage *image);
	bool write(const QImage &image);

	QByteArray name() const;

	static bool canRead(QIODevice *device);
};

#endif // QIMGIO_RLA_H
