--begin;
delete from job where status='deleted' and ( endedts < now()-'14 days'::interval or (endedts is null and submittedts < now()-'14 days'::interval) );
delete from jobassignment where fkeyjob not in (select keyjob from job);
delete from jobtask where fkeyjob not in(select keyjob from job);
delete from joberror where fkeyjob not in (select keyjob from job);
delete from jobhistory where fkeyjob not in (select keyjob from job);
delete from jobstatus where fkeyjob not in (select keyjob from job);
delete from jobservice where fkeyjob not in (select keyjob from job);
delete from jobdep where fkeyjob not in (select keyjob from job);
delete from joboutput where fkeyjob not in (select keyjob from job);

delete from jobtask where fkeyjob in
(select keyjob from job where status='deleted' and ( endedts < now()-'28 days'::interval or (endedts is null and submittedts < now()-'28 days'::interval) ));

delete from jobtaskassignment where keyjobtaskassignment in
(select keyjobtaskassignment
from jobtaskassignment
join jobassignment on fkeyjobassignment = keyjobassignment
where fkeyjob in
(select keyjob from job where status='deleted' and ( endedts < now()-'28 days'::interval or (endedts is null and submittedts < now()-'28 days'::interval) ))
)

delete from jobdep where fkeyjob in
(select keyjob from job where status='deleted' and ( endedts < now()-'28 days'::interval or (endedts is null and submittedts < now()-'28 days'::interval) ));

delete from joboutput where fkeyjob in
(select keyjob from job where status='deleted' and ( endedts < now()-'28 days'::interval or (endedts is null and submittedts < now()-'28 days'::interval) ));

delete from jobservice where fkeyjob in
(select keyjob from job where status='deleted' and ( endedts < now()-'28 days'::interval or (endedts is null and submittedts < now()-'28 days'::interval) ));

delete from jobstatus where fkeyjob in
(select keyjob from job where status='deleted' and ( endedts < now()-'28 days'::interval or (endedts is null and submittedts < now()-'28 days'::interval) ));

--commit;
vacuum full;
reindex database blur;
vacuum analyze;

