#include "jobstatustype.h"
#include "jobstatustypebase.h"

bool JobStatusType::operator==(const QString& rhs) const
{
	if (this->status() == rhs)
		return true;
	return false;
}
bool JobStatusType::operator!=(const QString& rhs) const
{
	if (this->status() != rhs)
		return true;
	return false;
}

bool JobStatusType::operator==(const Record& rhs) const
{
	if (this == &rhs)
		return true;
	return false;
}

bool JobStatusType::operator!=(const Record& rhs) const
{
	if (this != &rhs)
		return true;
	return false;
}

JobStatusType JobStatusType::statusTypeByStatus(const QString& rhs)
{
	if (pJobStatusTypes.size() == 0)
	{
		JobStatusTypeList jstl = JobStatusType::select();
		foreach (JobStatusType jst, jstl)
			pJobStatusTypes[jst.status().toLower()] = jst;
	}
	
	if (pJobStatusTypes.contains(rhs.toLower()))
		return pJobStatusTypes[rhs.toLower()];
	
	return JobStatusType();
}

QMap<QString, JobStatusType> JobStatusType::pJobStatusTypes;