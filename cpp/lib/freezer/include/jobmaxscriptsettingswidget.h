
/* $Author$
 * $LastChangedDate: 2010-02-16 17:20:26 +1100 (Tue, 16 Feb 2010) $
 * $Rev: 9358 $
 * $HeadURL: svn://svn.blur.com/blur/branches/concurrent_burn/cpp/lib/assfreezer/include/jobmaxscriptsettingswidget.h $
 */

#ifndef JOB_MAX_SCRIPT_SETTINGS_WIDGET_H
#define JOB_MAX_SCRIPT_SETTINGS_WIDGET_H

#include "job.h"

#include "jobsettingswidget.h"
#include "jobmaxsettingswidget.h"
#include "ui_jobmaxscriptsettingswidgetui.h"

namespace Stone {
	class RecordProxy;
}
using namespace Stone;

class FREEZER_EXPORT JobMaxScriptSettingsWidget : public CustomJobSettingsWidget, public Ui::JobMaxScriptSettingsWidgetUI, public JobMaxUtils
{
Q_OBJECT
public:
	JobMaxScriptSettingsWidget(QWidget * parent=0, JobSettingsWidget::Mode mode = JobSettingsWidget::ModifyJobs );
	~JobMaxScriptSettingsWidget();
	
	QStringList supportedJobTypes();
	static QStringList jobTypes();

public slots:
	void resetSettings();
	void applySettings();

protected:
	RecordProxy * mSelectedJobsProxy;
	bool mIgnoreChanges;
};

#endif // JOB_MAX_SCRIPT_SETTINGS_WIDGET_H
