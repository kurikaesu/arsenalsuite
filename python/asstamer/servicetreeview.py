
from blur.Stonegui import RecordTreeView

class ServiceTreeView(RecordTreeView):
	def __init__(self,parentWidget):
		RecordTreeView.__init__(self,parentWidget)
		self.setRootIsDecorated( True )
