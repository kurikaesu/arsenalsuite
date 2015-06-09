psql -h c100b09 -U shotgun int_drd_shotgun <<EOF2
COPY (select code as shotname, tasks.sg_darwin_score as complexity, 
CASE WHEN sg_display_name LIKE 'FX%' THEN 'hf2-fx'
     WHEN sg_display_name LIKE '%ight%' THEN 'hf2-light'
     ELSE sg_display_name
     END as projectname
FROM shots
join tasks on tasks.entity_id = shots.id
where tasks.sg_darwin_score > 1 and color = 'pipeline_step')
TO STDOUT
EOF2
