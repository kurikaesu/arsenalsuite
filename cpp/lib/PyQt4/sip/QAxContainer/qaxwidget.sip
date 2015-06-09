// This is the SIP interface definition for QAxWidget.
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

class QAxWidget : QWidget, QAxBase /PyQt4NoQMetaObject/
{
%TypeHeaderCode
#include <qaxwidget.h>
%End

public:
    QAxWidget(QWidget *parent /TransferThis/ = 0, Qt::WFlags f = 0);
    QAxWidget(const QString &c, QWidget *parent /TransferThis/ = 0,
            Qt::WFlags f = 0);
    //QAxWidget(IUnknown *iface, QWidget *parent /TransferThis/ = 0, Qt::WFlags f = 0);
    ~QAxWidget();

    void clear();
    bool doVerb(const QString &verb);

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    //virtual QaxAggregated *createAggregate();

protected:
    //bool initialize(IUnknown **);
    virtual bool createHostWindow(bool);

    void changeEvent(QEvent *e);
    void resizeEvent(QResizeEvent *);

    virtual bool translateKeyEvent(int,int) const;

    void connectNotify(const char *signal);
};

%End
