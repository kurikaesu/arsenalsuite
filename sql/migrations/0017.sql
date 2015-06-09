CREATE OR REPLACE VIEW license_usage_2 AS 
 SELECT service.service, license.total - license.reserved, count(ja.keyjobassignment) AS count
 FROM jobservice js
 JOIN service ON js.fkeyservice = service.keyservice
 LEFT JOIN jobassignment ja ON ja.fkeyjobassignmentstatus < 4 AND ja.fkeyjob = js.fkeyjob
 JOIN license ON service.fkeylicense = license.keylicense
 GROUP BY service.service, license.total, license.reserved;
