

if [ $# -lt 2 ]
then
	echo "This script needs a sql.gz backup file as an argument."
	exit 0
fi

gunzip -c $1 > /tmp/restore.sql
/etc/init.d/postgres stop
mv /var/lib/pgsql/data/pg_hba.conf /var/lib/pgsql/data/pg_hba.conf.restore
echo 'local   all     all                     trust' >> /var/lib/pgsql/data/pg_hba.conf
/etc/init.d/postgres start
dropdb -U postgres blur

psql template1 postgres < /tmp/restore.sql
rm /tmp/restore.sql
/etc/init.d/postgres stop
rm /var/lib/pgsql/data/pg_hba.conf
mv /var/lib/pgsql/data/pg_hba.conf.restore /var/lib/pgsql/data/pg_hba.conf
/etc/init.d/postgres start

