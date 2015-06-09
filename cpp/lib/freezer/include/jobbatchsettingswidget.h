
/* $Author$
 * $LastChangedDate: 2010-02-16 17:20:26 +1100 (Tue, 16 Feb 2010) $
 * $Rev: 9358 $
 * $HeadURL: svn://svn.blur.com/blur/branches/concurrent_burn/cpp/lib/assfreezer/include/jobbatchsettingswidget.h $
 */

#ifndef JOB_BATCH_SETTINGS_WIDGET_H
#define JOB_BATCH_SETTINGS_WIDGET_H

#include "job.h"

#include "jobsettingswidget.h"
#include "ui_jobbatchsettingswidgetui.h"

namespace Stone {
class RecordProxy;
}
using namespace Stone;

class FREEZER_EXPORT JobBatchSettingsWidget : public CustomJobSettingsWidget, public Ui::JobBatchSettingsWidgetUI
{
Q_OBJECT
public:
	JobBatchSettingsWidget(QWidget * parent=0, JobSettingsWidget::Mode mode = JobSettingsWidget::ModifyJobs );
	~JobBatchSettingsWidget();
	
	QStringList supportedJobTypes();
	static QStringList jobTypes();

public slots:
	void resetSettings();
	void applySettings();

protected:
	RecordProxy * mProxy;
};

#endif // JOB_BATCH_SETTINGS_WIDGET_H
