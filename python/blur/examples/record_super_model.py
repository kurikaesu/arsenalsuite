
import sys
#sys.argv += ['-db-host','localhost','-db-port','5433','-show-sql']
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from blur.Stone import *
from blur.Classes import *
from blur.Stonegui import *

app = QApplication([])

if sys.platform == 'win32':
	initConfig('c:/blur/assburner/assburner.ini')
else:
	initConfig('/etc/db.ini')

initStone(sys.argv)
blurqt_loader()

# Defines how a row holding a Job record will appear
class JobTrans(RecordDataTranslator):
	def __init__(self,tb):
		RecordDataTranslator.__init__(self,tb)
		# Use the builtin record column mapping to show all Job fields
		fields = QStringList()
		for f in Job.table().schema().fields():
			fields << f.name()
		self.setRecordColumnList(fields)
	
	# If we want to customize this trans we can implement
	# recordData, setRecordData, recordFlags, and compare
	def recordData(self,record,idx,role):
		if idx.column() == 0 and role==Qt.BackgroundRole:
			return QVariant(QColor(255,0,0))
		return RecordDataTranslator.recordData(self,record,idx,role)
	
# Defines how a row holding a JobCommandHistory record will appear
class HistoryTrans(RecordDataTranslator):
	def __init__(self,tb):
		RecordDataTranslator.__init__(self,tb)
		fields = QStringList()
		for f in JobCommandHistory.table().schema().fields():
			fields << f.name()
		self.setRecordColumnList(fields)
		
# The tree builder defines the parent/child relationship
class JobHistoryRTB(RecordTreeBuilder):
	def __init__(self,model):
		RecordTreeBuilder.__init__(self,model)
		# The first translator is the default
		self.JobTrans = JobTrans(self)
		self.HistoryTrans = HistoryTrans(self)
		
	# We could check to see if the job actually has any history records, but
	# that would result in loading them all from the db, which could be slow
	# if we are showing a lot of records.  Always returning true allows
	# them to be loaded on demand
	def hasChildren(self, parentIndex, model):
		return isinstance(model.getRecord(parentIndex),Job)
	
	# Load the JobCommandHistory records for the job, and insert them
	# with the HistoryTrans translator
	def loadChildren(self, parentIndex, model):
		rec = model.getRecord(parentIndex)
		if isinstance(rec,Job):
			histories = JobCommandHistory.select( "fkeyjob=?", [QVariant(rec.key())] )
			self.HistoryTrans.appendRecordList( histories, parentIndex )

# Setup the view
rtv = RecordTreeView(None)
rtv.setRootIsDecorated(True)

# Setup the model
# If we were going to use this same model in multiple places, we
# could subclass RecordSuperModel and do this setup in it's __init__
# function, then it would only take a single line to setup a view
# with this model, AND HistoryTrans, JobTrans, and  would still be
# usable by themselves
rsm = RecordSuperModel(rtv)
rtb = JobHistoryRTB(rsm)
rsm.setTreeBuilder(rtb)
# Use the Job columns as the header labels
columns = QStringList()
for f in Job.table().schema().fields():
	columns << f.displayName()
rsm.setHeaderLabels( columns )

# Insert using the default(JobTrans) translator
rsm.insert( JobFusionVideoMaker.select() )

# Tell the view to use the model
# Possible to use the same model on multiple view at the same time
rtv.setModel(rsm)
rtv.show()

app.exec_()
