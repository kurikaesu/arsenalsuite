// This is the SIP interface definition for QAxBase.
//
// Copyright (c) 2009 Riverbank Computing Limited <info@riverbankcomputing.com>
// 
// This file is part of PyQt.
// 
// This file may be used under the terms of the GNU General Public
// License versions 2.0 or 3.0 as published by the Free Software
// Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
// included in the packaging of this file.  Alternatively you may (at
// your option) use any later version of the GNU General Public
// License if such license has been publicly approved by Riverbank
// Computing Limited (or its successors, if any) and the KDE Free Qt
// Foundation. In addition, as a special exception, Riverbank gives you
// certain additional rights. These rights are described in the Riverbank
// GPL Exception version 1.1, which can be found in the file
// GPL_EXCEPTION.txt in this package.
// 
// Please review the following information to ensure GNU General
// Public Licensing requirements will be met:
// http://trolltech.com/products/qt/licenses/licensing/opensource/. If
// you are unsure which license is appropriate for your use, please
// review the following information:
// http://trolltech.com/products/qt/licenses/licensing/licensingoverview
// or contact the sales department at sales@riverbankcomputing.com.
// 
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.


%If (WS_WIN)

class QAxBase /Abstract, PyQt4NoQMetaObject/
{
%TypeHeaderCode
#include <qaxbase.h>
%End

public:
    //QAxBase(IUnknown *iface = 0);
    virtual ~QAxBase();

    QString control() const;

    //long queryInterface(const QUuid &, void **) const;

    QVariant dynamicCall(const char *name,
            const QVariant &v1 = QVariant(), const QVariant &v2 = QVariant(),
            const QVariant &v3 = QVariant(), const QVariant &v4 = QVariant(),
            const QVariant &v5 = QVariant(), const QVariant &v6 = QVariant(),
            const QVariant &v7 = QVariant(), const QVariant &v8 = QVariant());
    QVariant dynamicCall(const char *name, QList<QVariant> &vars);

    QAxObject *querySubObject(const char *name,
            const QVariant &v1 = QVariant(), const QVariant &v2 = QVariant(),
            const QVariant &v3 = QVariant(), const QVariant &v4 = QVariant(),
            const QVariant &v5 = QVariant(), const QVariant &v6 = QVariant(),
            const QVariant &v7 = QVariant(), const QVariant &v8 = QVariant());
    QAxObject *querySubObject(const char *name, QList<QVariant> &vars);

    // SIP has a bug triggered by a template definition being the subject of
    // multiple typedefs.  It only really matters when building everything as
    // one big module (the code that implements the type is duplicated in
    // other cases).  Until it is fixed we just avoid the problematic typedef.
    //typedef QMap<QString, QVariant> PropertyBag;
    //PropertyBag propertyBag() const;
    //void setPropertyBag(const PropertyBag &);
    QVariantMap propertyBag() const;
    void setPropertyBag(const QVariantMap &);

    QString generateDocumentation();

    virtual bool propertyWritable(const char *) const;
    virtual void setPropertyWritable(const char *, bool);

    bool isNull() const;

    QStringList verbs() const;

    QVariant asVariant() const;

signals:
    void signal(const QString &, int, void *);
    void propertyChanged(const QString &);
    void exception(int, const QString &, const QString &, const QString &);

public:
    virtual void clear();
    bool setControl(const QString &);

    void disableMetaObject();
    void disableClassInfo();
    void disableEventSink();

protected:
    //virtual bool initialize(IUnknown** ptr);
    //bool initializeRemote(IUnknown** ptr);
    //bool initializeLicensed(IUnknown** ptr);
    //bool initializeActive(IUnknown** ptr);
    //bool initializeFromFile(IUnknown** ptr);
};

%End
