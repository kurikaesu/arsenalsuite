#!/usr/bin/python
from PyQt4.QtCore import *
from PyQt4.QtSql import *
from blur.Stone import *
from blur.Classes import *
import sys

# First Create a Qt Application
app = QCoreApplication(sys.argv)

# Load database config
if sys.platform=='win32':
	initConfig("c:\\blur\\resin\\resin.ini")
else:
	initConfig("/etc/db.ini")
blurqt_loader()

FreezerCore.instance().reconnect()

def reindex_table( table ):
	q = QSqlQuery()
	q.prepare('REINDEX TABLE %s' % table.tableName())
	if q.exec_():
		Log( 'Successfully Re-Indexed %s' % table.tableName() )
	else:
		Log( 'Error Re-Indexing %s' % table.tableName() )

for table in Database.instance().tables():
	reindex_table( table )

