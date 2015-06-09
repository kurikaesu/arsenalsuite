/*
 * Copyright (c) 2009 Dr. D Studios. (Please refer to license for details)
 * SVN_META_HEADURL = "$HeadURL: $"
 * SVN_META_ID = "$Id: BachDisplayOptions.h 9408 2010-03-03 22:35:49Z brobison $"
 */

#ifndef _DRD_BACHDISPLAYOPTIONS_H_
#define _DRD_BACHDISPLAYOPTIONS_H_

#include <qframe.h>
#include "ui_bachdisplayoptions.h"

//---------------------------------------------------------------------------------------------
//
class BachDisplayOptions : public QFrame, public Ui_BachDisplayOptions
{
Q_OBJECT
public:
	BachDisplayOptions( QWidget * parent );
	virtual ~BachDisplayOptions() { }
    QSize tnSize() const;

protected slots:
    void tnSizeChanged( const QString & );

private:
    void initThumbSlider();
};

#endif /* _DRD_BACHDISPLAYOPTIONS_H_ */
