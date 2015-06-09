
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of Arsenal.
 *
 * Arsenal is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Arsenal is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Blur; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* $Author$
 * $LastChangedDate: 2008-04-17 14:13:22 -0700 (Thu, 17 Apr 2008) $
 * $Rev: 6353 $
 * $HeadURL: svn://newellm@ocelot/blur/trunk/cpp/lib/assfreezer/include/jobrealflowsettingswidget.h $
 */

#ifndef JOBREALFLOWSETTINGS_WIDGET_H
#define JOBREALFLOWSETTINGS_WIDGET_H

#include "job.h"

#include "jobsettingswidget.h"
#include "ui_jobrealflowsettingswidgetui.h"

namespace Stone {
	class RecordProxy;
}
using namespace Stone;

class FREEZER_EXPORT JobRealFlowSettingsWidget : public CustomJobSettingsWidget, public Ui::JobRealFlowSettingsWidgetUI
{
Q_OBJECT
public:
	JobRealFlowSettingsWidget(QWidget * parent=0, JobSettingsWidget::Mode mode = JobSettingsWidget::ModifyJobs );
	~JobRealFlowSettingsWidget();
	
	QStringList supportedJobTypes();
	static QStringList jobTypes();

public slots:
	void resetSettings();
	void applySettings();

protected:
	RecordProxy * mProxy;
};

#endif // JOBREALFLOWSETTINGS_WIDGET_H


