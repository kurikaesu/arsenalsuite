
/* $Author$
 * $LastChangedDate: 2007-06-19 04:27:47 +1000 (Tue, 19 Jun 2007) $
 * $Rev: 4632 $
 * $HeadURL: svn://svn.blur.com/blur/branches/concurrent_burn/cpp/lib/assfreezer/include/jobstatwidget.h $
 */


#ifndef JOB_STAT_WIDGET_H
#define JOB_STAT_WIDGET_H

#include <qtreewidget.h>

#include "job.h"

class JobStatWidget : public QTreeWidget
{
Q_OBJECT
public:
	JobStatWidget( QWidget * parent = 0 );

	void setJobs( const JobList & );
	JobList jobs() const;

	void refresh();

protected:
	JobList mJobs;

};

#endif // JOB_STAT_WIDGET_H
