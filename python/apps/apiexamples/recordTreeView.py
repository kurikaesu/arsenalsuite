from PyQt4.QtCore import *
from PyQt4.QtGui import *


from blur.Stone import *
from blur.Stonegui import *
from blur.Classes import *

import sys, os

schema = None

# Generates a temporary table schema for use in the tree view
def projectReserveTable():
    global schema
    if schema == None:
        schema = TableSchema(classesSchema())
        Field(schema, "fkeyproject", "Project" )
        Field(schema, "arsenalSlotReserve", Field.Double)
        Field(schema, "currentAllocation", Field.Double)

    ret = schema.table()
    return ret

# RecordItemBase derived
class ProjectReserveItem(SipRecordItemBase):
    def __init__(self, builder):
        super(ProjectReserveItem, self).__init__(builder)
        self.record = []
        self.project = []

    def setup(self, idx, r):
        self.record.append(r)
        self.project.append(Project(r.foreignKey("fkeyproject")))

    def modelData(self, idx, role):
        col = idx.column()
        row = idx.row()
        if( role == Qt.DisplayRole or role == Qt.EditRole ):
            if col == 0:
                return QVariant(self.project[row].name())
            elif col == 1:
                return QVariant(self.record[row].getValue("arsenalSlotReserve"))
            elif col == 2:
                return QVariant(self.record[row].getValue("currentAllocation"))

        return QVariant()

    def setModelData(self, idx, val, role):
        if( role == Qt.EditRole and idx.column() == 1):
            self.project[idx.row()].setArsenalSlotReserve(val.toDouble()[0])
            self.project[idx.row()].commit()
            self.record[idx.row()].setValue("arsenalSlotReserve", val)
            return True

        return False

    def getRecord(self, idx):
        return self.record[idx.row()]

    def modelFlags(self, ptr, idx):
        if( idx.column() == 1 ):
            return Qt.ItemFlags(Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsEditable)
        else:
            return Qt.ItemFlags(Qt.ItemIsEnabled | Qt.ItemIsSelectable)

# The record data translator for the tree view
class ProjectTranslator(SipRecordDataTranslatorBase, ProjectReserveItem):
    def __init__(self, builder):
        super(ProjectTranslator, self).__init__(builder)

    def modelData(self, ptr, idx, role):
        ret = ProjectReserveItem.modelData(self, idx, role)
        if( not ret.isValid() ):
            return SipRecordDataTranslatorBase.recordData(self, ProjectReserveItem.getRecord(self, idx), idx, role)
        return ret

    def setModelData(self, ptr, idx, val, role):
        ret = ProjectReserveItem.setModelData(self, idx, val, role)
        if( not ret ):
            return SipRecordDataTranslatorBase.setRecordData(self, ProjectReserveItem.getRecord(self, idx), idx, val, role)
        return ret

    def insertRecordList(self, row, rl, parent):
        if( rl.size() <= 0): return QModelIndexList()
        model = self.model()
        SuperModel.InsertClosure(model)
        ret = model.insert(parent, row, rl.size(), self)
        for i in range(rl.size()):
            ProjectReserveItem.setup(self, ret[i], rl[i])

        return ret

    def getRecord(self, idx):
        return ProjectReserveItem.getRecord(self, idx)

# The tree view itself
class ProjectReserveView(RecordTreeView):
    def __init__(self, parent):
        RecordTreeView.__init__(self, parent)
        self.model = RecordSuperModel(self)
        self.translator = ProjectTranslator(self.model.treeBuilder())
        self.model.setHeaderLabels( ["Project", "Reserve", "Current Allocation"] )
        self.setModel(self.model)
        self.model.sort(1)
        print self.selectionModel()

    def refresh(self):
        self.records = RecordList()
        self.recordlist = []
        q = Database.current().exec_("select project.keyelement, project_slots_current.arsenalslotreserve, sum from project_slots_current join project on project.name=project_slots_current.name;")

        while q.next():
            self.records.append( Record( RecordImp( q, projectReserveTable() ) ) )

        self.model.setRootList( self.records )

class RecordTreeViewTest:
    def __init__(self):
        self.app = QApplication([""])
        
        initConfig(os.getenv('ABSUBMIT', '.') + "/ab.ini", os.getenv('TEMP', '/tmp') + "/recordtreeviewtest.log")
        initStone(sys.argv)
        classes_loader()

        self.window = QMainWindow()
        self.window.show()

        prview = ProjectReserveView(self.window)
        self.window.setCentralWidget(prview)

        prview.refresh()

    def run(self):
        self.app.exec_()

if __name__ == "__main__":
    rtvt = RecordTreeViewTest()
    rtvt.run()
