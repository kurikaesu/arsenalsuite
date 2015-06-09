
#!/usr/bin/python

from blur.Stone import *

#Sharing a schema between the two databases/connections
schema = Schema.createFromXmlSchema( '/mnt/storage/blur/python/apps/trax/definitions/schema.xml' )

# Create a live db connection and database
c_live = Connection.create( 'QPSQL7' )
c_live.setHost( 'lion' )
c_live.setPort( 5432 )
c_live.setDatabaseName( 'blur' )
c_live.setUserName( 'brobison' )
c_live.reconnect()
d_live = Database( schema, c_live )
d_live.setEchoMode( Database.EchoInsert | Database.EchoUpdate | Database.EchoDelete )

# Create a trax db connection and database
c_trax = Connection.create( 'QPSQL7' )
c_trax.setHost( 'lion' )
c_trax.setPort( 5432 )
c_trax.setDatabaseName( 'trax_beta' )
c_trax.setUserName( 'trax' )
c_trax.reconnect()
d_trax = Database( schema, c_trax )

# Select userrole entries from trax
user_roles = d_trax.tableByName('userrole').select()

# Migrate them to live
# This part should be easier
# Should be able to make a copy automatically from one table to the next
# with the primary key optionally cleared
migrated_roles = RecordList()
userrole_live = d_live.tableByName('userrole')
for role in user_roles:
	migrated = userrole_live.load()
	for f in userrole_live.schema().fields():
		if not f.flag(Field.PrimaryKey):
			migrated.setValue( f.pos(), role.getValue( f.pos() ) )
	migrated_roles += migrated

print migrated_roles.dump()
print migrated_roles.size()
migrated_roles.commit()
