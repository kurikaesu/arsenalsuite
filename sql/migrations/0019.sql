CREATE OR REPLACE VIEW user_service_current AS 
    SELECT (usr.name || ':'::text) || service.service, sum(ja.assignslots) AS sum
    FROM usr
    JOIN job ON usr.keyelement = job.fkeyusr AND (job.status = ANY (ARRAY['ready'::text, 'started'::text]))
    JOIN jobassignment ja ON job.keyjob = ja.fkeyjob AND ja.fkeyjobassignmentstatus < 4
    JOIN jobservice js ON job.keyjob = js.fkeyjob
    JOIN service ON service.keyservice = js.fkeyservice
    GROUP BY usr.name, service.service;
