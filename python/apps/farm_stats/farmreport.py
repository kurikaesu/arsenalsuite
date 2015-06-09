
from blur.Stone import *
from blur.Stonegui import *
from PyQt4.QtCore import *
from blur.defaultdict import *

class FarmReport:
	def __init__(self,name):
		self.name = name
		self.records = RecordList()
		self.columns = []
		self.headerLabels = []
		self.model = None
		
	def generate(self,startDate,endDate):
		self.startDate = startDate
		self.endDate = endDate
		pass

	def createModel( self, parent = None ):
		ret = RecordListModel(parent)
		self.setupModel(ret)
		return ret
	
	def setupModel( self, model ):
		model.setRootList( self.records )
		model.setColumns( self.columns )
	
user_slave_summary_table = None
user_render_summary_table = None

def loadTables():
	global user_slave_summary_table
	global user_render_summary_table
	global user_job_count_table
	# Create the table for this function
	if not user_slave_summary_table:
		Database.current().schema().mergeXmlSchema( 'stats_schema.xml' )
		user_slave_summary_table = Database.current().tableByName( 'user_slave_summary' )
		user_render_summary_table = Database.current().tableByName( 'user_render_summary' )
		user_job_count_table = Database.current().tableByName( 'user_job_counts' )

class UserHostSlaveReportModel(RecordListModel):
	def __init__(self,parent = None):
		RecordListModel.__init__(self,parent)
		
	def compare( self, r1, c1, r2, c2 ):
		if c1.toLower() == 'hours':
			d1 = r1.getValue(c1).toDouble()[0]
			d2 = r2.getValue(c2).toDouble()[0]
			if d1 > d2: return 1
			if d2 > d1: return -1
			return 0
		return RecordListModel.compare(self,r1,c1,r2,c2)

class UserHostSlaveReport(FarmReport):
	def __init__(self):
		FarmReport.__init__(self,'User Host Slave Summary')
		self.columns = ['User', 'Host', 'Hours']
		
	def generate(self,startDate,endDate):
		FarmReport.generate(self,startDate,endDate)
		loadTables()
		stats = Database.current().exec_( "SELECT * FROM hosthistory_user_slave_summary( ?, ? );", [QVariant(startDate),QVariant(endDate)] );
		while stats.next():
			r = Record( RecordImp( user_slave_summary_table, stats ), False )
			self.records.append( r )
			print r.getValue( 'user' ).toString(), r.getValue( 'host' ).toString(), r.getValue( 'hours' ).toString()

	def createModel( self, parent = None ):
		ret = UserHostSlaveReportModel(parent)
		self.setupModel(ret)
		return ret

class UserRenderReportModel(RecordListModel):
	def __init__(self,parent = None):
		RecordListModel.__init__(self,parent)
		self.IntervalCols = ['totalrendertime','totalerrortime']
		
	def compare( self, r1, c1, r2, c2 ):
		if c1.toLower() in self.IntervalCols:
			i1 = Interval.fromString(r1.getValue(c1).toString())[0]
			i2 = Interval.fromString(r2.getValue(c2).toString())[0]
			return Interval.compare( i1, i2 )
		return RecordListModel.compare(self,r1,c1,r2,c2)
	
	def recordData( self, record, role, col ):
		if role == Qt.DisplayRole and col.toLower() in self.IntervalCols:
			i = Interval.fromString(record.getValue(col).toString())[0]
			return QVariant(i.toString(Interval.Hours,Interval.Hours))
		return RecordListModel.recordData(self,record,role,col)
			
class UserRenderReport(FarmReport):
	def __init__(self):
		FarmReport.__init__(self,'User Render Summary')
		self.columns = ['User', 'TotalRenderTime', 'TotalErrorTime', 'ErrorTimePercent']
		
	def generate(self,startDate,endDate):
		FarmReport.generate(self,startDate,endDate)
		loadTables()
		stats = Database.current().exec_( "select usr.name, sum(coalesce(totaltasktime,'0'::interval)) as totalrendertime, sum(coalesce(totalerrortime,'0'::interval)) as totalerrortime, sum(coalesce(totalerrortime,'0'::interval))/sum(coalesce(totaltasktime,'0'::interval)+coalesce(totalerrortime,'0'::interval)) as errortimeperc from jobstat, usr where started > 'today'::timestamp - '7 days'::interval and ended is not null and fkeyusr=keyelement group by usr.name order by errortimeperc desc;" )
		while stats.next():
			r = Record( RecordImp( user_render_summary_table, stats ), False )
			self.records.append( r )
			print r.getValue( 'user' ).toString(), r.getValue( 'totalRenderTime' ).toString(), r.getValue( 'errortimepercent' ).toString()

	def createModel( self, parent = None ):
		ret = UserRenderReportModel(parent)
		self.setupModel(ret)
		return ret
	
class UserJobCountReport(FarmReport):
	def __init__(self):
		FarmReport.__init__(self,'User Job Counts')
		self.columns = ['User','Ready','Started','Done','Suspended','Holding']
		
	def generate(self,startDate,endDate):
		FarmReport.generate(self,startDate,endDate)
		loadTables()
		per_user = {}
		stats = Database.current().exec_("select name, job.status, count(*) from job, usr where fkeyusr=keyelement group by keyelement, job.status, name order by job.status, count desc;")
		while stats.next():
			user = stats.value(0).toString()
			if not user in per_user:
				per_user[user] = Record( RecordImp( user_job_count_table ), False )
				per_user[user].setValue( "user", QVariant(user) )
			per_user[user].setValue( stats.value(1).toString(), QVariant(stats.value(2).toInt()[0]) )
		for rec in per_user.values():
			self.records.append( rec )
	
class FarmReportType:
	def __init__(self, reportName, reportClass):
		self.Name = reportName
		self.Class = reportClass

Types = [FarmReportType("User's Host Slave Report",UserHostSlaveReport), FarmReportType("User Render Report",UserRenderReport), FarmReportType("User Job Count Report", UserJobCountReport)]
