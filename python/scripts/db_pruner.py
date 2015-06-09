
from blur.quickinit import *

def tableSizeInMegs(tableName):
	q = Database.current().exec_('SELECT * FROM table_size_in_megs(?)',[QVariant(tableName)])
	if q.next(): return q.value(0)
	return None
	

def pruneTable(tableName, defaultSizeLimit, orderColumn, rowsPerIteration = 100):
	maxSize = Config.getInt('assburnerTableLimit' + tableName, defaultSizeLimit)
	tableNameLwr = str(tableName).lower()
	while tableSizeInMegs(tableNameLwr) > maxSize:
		q = Database.current().exec_('DELETE FROM %(name)s WHERE key%(name)s IN (SELECT key%(name)s FROM %(name)s ORDER BY %(sort_col)s DESC LIMIT %(limit)i)' % {'name':tableNameLwr,'sort_col':orderColumn,'limit':rowsPerIteration} )
		if q.numRowsAffected() < rowsPerIteration:
			break

pruneTable('JobCommandHistory', 5000, 'keyjobcommandhistory')
	