
import sys
import operator
import webbrowser
import datetime, time

from PyQt4.QtCore import *
from PyQt4.QtGui  import *
from ui.ui_afterburner import Ui_Afterburner
from ui.ui_preferences import Ui_Preferences

from blur.Stone   import *
from blur.Classes import *

from socket import gethostname

# Get the hostname
HOSTNAME = sys.argv[1] if len(sys.argv) > 1 else gethostname().split('.')[0]

class HostData:
    
    def __init__(self):
        
        self.host = Host.select( "host = '%s'" % HOSTNAME )[0]

    def __call__(self):
        return self

    def host(self):

        return self.host


class TableModel(QAbstractTableModel):
    def __init__(self, datain, headerdata, parent=None, *args):
        """ datain: a list of lists
            headerdata: a list of strings
        """
        QAbstractTableModel.__init__(self, parent, *args)
        self.hostData   = HostData()
        self.arraydata  = datain
        self.headerdata = headerdata
        self.refresh()

    def rowCount(self, parent):
        return len(self.arraydata)

    def arrayData(self, index):
        return self.arraydata[index]

    def columnCount(self, parent):
        return len(self.arraydata[0])

    def data(self, index, role):
        if role == Qt.TextAlignmentRole:
            return Qt.AlignCenter

        if not index.isValid():
            return QVariant()
        elif role != Qt.DisplayRole:
            return QVariant()

        return QVariant(self.arraydata[index.row()][index.column()])

    def refresh(self):

        self.arraydata = []

        assignments = JobAssignment.select("INNER JOIN Host ON fkeyhost=keyhost WHERE fkeyjobassignmentstatus=3 AND host='%s'" % HOSTNAME)
        self.slots  =  0
        jobs        = []
        for assignment in assignments:

            # Get the number of slots in use
            self.slots += assignment.assignSlots()

            # Get the memory for the assignment
            taskassignment = JobTaskAssignment.select("fkeyjobassignment=%s" % assignment.key())[0]

            # Host slots
            hostslots = self.hostData.host.maxAssignments()

            jobStatus        = JobStatus.select("fkeyjob=%s" % assignment.job().key())[0]
            running          = int(time.time()) - assignment.started().toTime_t()
            tasksAverageTime = jobStatus.tasksAverageTime()
            tasksAverageTime = tasksAverageTime/assignment.assignSlots()
            remaining        = tasksAverageTime - running
            eta              = "%s" % datetime.timedelta(seconds=remaining) if remaining > 0 else "N/A"

            self.arraydata.append((
                assignment.key(),
                "%s/%s" % (assignment.assignSlots(), hostslots),
                "%s MB" % str(int(taskassignment.memory()) / 1024),
                "%s" % datetime.timedelta(seconds=running),
                eta,
                assignment.job().name(),
            ))

        if len(self.arraydata) == 0: self.arraydata.append(('', '', '', '', '', ''))

        self.emit(SIGNAL("layoutChanged()"))

    def getSlots(self):
        return self.slots

    def headerData(self, col, orientation, role):
        if orientation == Qt.Horizontal and role == Qt.DisplayRole:
            return QVariant(self.headerdata[col])
        return QVariant()

    def sort(self, Ncol, order):
        """Sort table by given column number.
        """
        self.emit(SIGNAL("layoutAboutToBeChanged()"))
        self.arraydata = sorted(self.arraydata, key=operator.itemgetter(Ncol))
        if order == Qt.DescendingOrder:
            self.arraydata.reverse()
        self.emit(SIGNAL("layoutChanged()"))

class Preferences(QDialog, Ui_Preferences):

    def __init__(self, parent=None):
        QDialog.__init__(self, parent)

        self.hostData = HostData()
        self.setupUi(self)
        self.connect(self, SIGNAL('triggered()'), self.closeEvent)

    def setSlotBox(self):

        # Set the slot label
        self.slotLabel.setText("Slots. (%s Available)" % self.hostData.host.cpus())

        # Set the maximum this spinbox can hold
        self.slotBox.setMaximum(self.hostData.host.cpus())

        # Set the current maxAssignments
        self.slotBox.setValue(self.hostData.host.maxAssignments())

    def closeEvent(self, event):

        # Save changes
        self.hostData.host.setMaxAssignments(self.slotBox.value()).commit()

        self.deleteLater()

class Afterburner(QMainWindow, Ui_Afterburner):

    def __init__(self):
        QMainWindow.__init__(self)
        self.setupUi(self)

        # Get host data
        self.hostData = HostData()

        # Set jobBox title
        self.jobBox.setTitle("Running jobs on host: %s" % HOSTNAME)

        # Toolbar
        self.createToolbar()

        # Hook into close event
        self.connect(self, SIGNAL('triggered()'), self.closeEvent)

        # Set up tray icon
        self.w = QWidget()
        self.trayIcon = SystemTrayIcon(QIcon("icons/server_offline.png"), self.w)
        self.trayIcon.show()
        traySignal = "activated(QSystemTrayIcon::ActivationReason)"
        self.trayIcon.connect(self.trayIcon, SIGNAL(traySignal), self.iconActivated)

        #self.menu = QMenu()
        #self.exitAction = self.menu.addAction(QIcon("icons/door_out.png"), "Exit")
        #self.exitAction.connect(self.exitAction, SIGNAL('triggered()'), self.exitEvent)
        #self.trayIcon.setContextMenu(self.menu)

        # Create the jobTable
        self.createJobTable()

        self.score = QLabel()
        self.score.setTextFormat(Qt.RichText);
        self.score.setText("<img src=\"icons/award_1.png\" /> ")
        self.statusBar().addPermanentWidget(self.score)

        # Setup UI
        self.refreshUi()

        # Disable stop button initially
        self.stopSelected.setDisabled(True)

        # Bottom right on screen
        resolution = QDesktopWidget().screenGeometry()
        self.move((resolution.width()) - (self.frameSize().width()), (resolution.height()) - (self.frameSize().height()))

        # Set up timer for periodic refreshes
        self.timer = QTimer()
        self.connect(self.timer, SIGNAL("timeout()"), self.refreshUi)
        self.timer.start(60000)

        # Set up timer for utilization indicator
        # Check every 2 hours
        self.utilizationTimer = QTimer()
        self.connect(self.utilizationTimer, SIGNAL("timeout()"), self.checkUtilization)
        self.utilizationTimer.start(7200000)

        self.checkUtilization()

    def exitEvent(self):
        app.exit()

    def iconActivated(self, event):
        if event == QSystemTrayIcon.Trigger:
            self.showNormal()
            self.activateWindow()
            self.raise_()

    def closeEvent(self, event):

        # Minimize to System Tray
        self.hide()
        event.ignore()

    def changeEvent(self, event):

        # If minimized just hide, no need to keep in taskbar
        if event.type() == QEvent.WindowStateChange:
            if self.isMinimized():
                self.hide()

    def helpSlot(self):
        webbrowser.open("http://prodwiki/mediawiki/index.php/Farm:Desktop_Render_Interface")

    def settingsSlot(self):

        self.dialog = Preferences()
        self.dialog.show()

        self.dialog.setSlotBox()

    def jobSelectionChanged(self, qmodelindex):

        rows = self.jobView.selectionModel().selectedRows()

        self.selectedAssignments = []
        if len(rows) > 0 and rows[0].data(Qt.DisplayRole).toString() != '':
            self.stopSelected.setDisabled(False)

            for row in rows:
                assignment = JobAssignment.select("keyjobassignment=%s" % row.data(Qt.DisplayRole).toString())
                self.selectedAssignments.append(assignment[0])
        else:
            self.stopSelected.setDisabled(True)

    def refreshUi(self):

        # Refresh the TableModel
        self.jobModel.refresh()

        # Job View selection changes
        self.connect(self.jobView.selectionModel(), SIGNAL('selectionChanged(const QItemSelection&, const QItemSelection&)'), self.jobSelectionChanged)

        self.setToggleButton()

        self.trayIcon.setToolTip("Status: %s" % self.hostData.host.hostStatus().slaveStatus())

        self.setStatus()

        #self.score.setText("<img src=\"icons/award_1.png\" /> <img src=\"icons/award_2.png\" /> <img src=\"icons/award_3.png\" />")

        # Set row height
        nrows = len(self.jobModel.arraydata)
        for row in xrange(nrows):
            self.jobView.setRowHeight(row, 18)

    def checkUtilization(self):

        # Get the possible render seconds for 7 days
        # Take half of that to be more realistic ...
        possibleRenderSeconds = (self.hostData.host.cpus() * 60 * 60 * 24 * 7) / 2

        # Get number of seconds since last Monday
        tdate        = datetime.date.today()
        monday       = tdate - datetime.timedelta(days=tdate.weekday())
        mondayStr    = monday.strftime("%Y-%m-%d")
        difference   = datetime.datetime.now() - datetime.datetime.strptime(mondayStr + " 00:00:00", "%Y-%m-%d %H:%M:%S")
        secsSinceMon = (difference.days * 60 * 60 * 24) + difference.seconds

        # Query for host activity since Monday
        query = """select sum( extract('epoch' from coalesce(ja.ended,now()) - greatest( ja.started, (now()-'%ss'::interval)::timestamp ))*ja.assignslots )::integer
        from jobassignment ja
        where ja.fkeyhost=%s
        and ja.ended > now()-'%ss'::interval
        and ja.ended > ja.started
        """ % (secsSinceMon, self.hostData.host.key(), secsSinceMon)
        
        result      = Database.current().exec_(query)
        ulilization = 0
        if result.next():
            utilization = result.value(0).toInt()[0]
       
        # Get percentage of utilization 
        percent = int((utilization / float(possibleRenderSeconds)) * 100)

        # Give a new award every 7%
        award = int(5 * round(float(percent)/5))

        self.score.setText("<img src=\"icons/award_%s.png\" /> " % award)
        #self.score.setToolTip("Current Utilization: " + str(utilization) + "/" + str(possibleRenderSeconds))
        self.score.setToolTip("Utilization since Monday: " + str(utilization / 60 / 60) + " / " + str(possibleRenderSeconds / 60 / 60) + " Render Hours ( " + str(percent) + "% )")

    def setStatus(self):
        status = "Status: " + self.hostData.host.hostStatus().reload().slaveStatus()
        self.statusBar().showMessage(status)

    def createToolbar(self):

        self.toolbar = self.addToolBar('Toolbar')
        self.toolbar.setFixedHeight(32)
        self.toolbar.setIconSize(QSize(16, 16))

        self.refresh = QAction(QIcon('icons/arrow_refresh.png'), 'Refresh', self)
        self.refresh.setShortcut('Ctrl+R')
        self.connect(self.refresh, SIGNAL('triggered()'), self.refreshUi)
        self.toolbar.addAction(self.refresh)

        self.toolbar.addSeparator()

        self.farm = QAction(self)
        self.farm.setIcon(QIcon('icons/server_delete.png'))
        self.farm.setText('Take Host off Farm')
        self.connect(self.farm, SIGNAL('triggered()'), self.farmSlot)
        self.toolbar.addAction(self.farm)

        self.stopSelected = QAction(QIcon('icons/cancel.png'), 'Stop Selected Jobs', self)
        self.connect(self.stopSelected, SIGNAL('triggered()'), self.stopSelectedJobsSlot)
        self.toolbar.addAction(self.stopSelected)

        self.toolbar.addSeparator()

        self.statisticsAction = QAction(QIcon('icons/chart_bar.png'), 'Host Statistics', self)
        self.connect(self.statisticsAction, SIGNAL('triggered()'), self.showStatistics)
        self.toolbar.addAction(self.statisticsAction)

        self.settings = QAction(QIcon('icons/cog.png'), 'Host Settings', self)
        self.connect(self.settings, SIGNAL('triggered()'), self.settingsSlot)
        self.toolbar.addAction(self.settings)

        self.spacer = QWidget()
        self.spacer.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.toolbar.addWidget(self.spacer)

        self.help = QAction(QIcon('icons/help.png'), 'Help', self)
        self.connect(self.help, SIGNAL('triggered()'), self.helpSlot)
        self.toolbar.addAction(self.help)

    def setToggleButton(self):

        self.status = self.hostData.host.hostStatus().slaveStatus()

        if self.status in ('offline', 'stopping'):
            self.farm.setIcon(QIcon('icons/server_add.png'))
            self.farm.setText('Join Host to Farm')

            self.trayIcon.setIcon(QIcon("icons/server_offline.png"))
            self.trayIcon.setToolTip("Status: %s" % self.hostData.host.hostStatus().slaveStatus())
        else:
            self.farm.setIcon(QIcon('icons/server_delete.png'))
            self.farm.setText('Take Host off Farm')

            #self.trayIcon.setIcon(QIcon("icons/server_ready.png"))
            #noOfAssignments = 0
            #if self.jobModel.rowCount(None) > 0 and self.jobModel.arrayData(0)[0] != '':
            #    noOfAssignments = self.jobModel.rowCount(None)

            self.trayIcon.setIcon(QIcon("icons/server_ready_" + str(self.jobModel.getSlots()) + ".png"))
            self.trayIcon.setToolTip("Status: %s" % self.hostData.host.hostStatus().slaveStatus())

    def showStatistics(self):
        webbrowser.open("http://ganglia?c=Workstations&h=" + HOSTNAME + ".drd.int&m=load_one&r=hour&s=descending&hc=4&mc=2")

    def farmSlot(self):

        if self.hostData.host.hostStatus().slaveStatus() == 'offline':
            self.hostData.host.hostStatus().setSlaveStatus("ready").commit()
        else:
            self.hostData.host.hostStatus().setSlaveStatus("offline").commit()

        self.refreshUi()

    def stopSelectedJobsSlot(self):

        for assignment in self.selectedAssignments:

            # Stop the selected task
            Database.current().exec_( "SELECT cancel_job_assignment(%i)" % assignment.key() )

        self.refreshUi()

    def createJobTable(self):

        header = ('Task Id', 'Slots', 'Memory', 'Time', 'ETA', 'Job Name')

        self.jobModel = TableModel([], header, self)

        self.jobView.setModel(self.jobModel)
        self.jobView.setShowGrid(False)

        # Hide task id column
        self.jobView.hideColumn(0)

        # Set Font
        font = QFont("Courier New", 8)
        self.jobView.setFont(font)

        # Set Alternating rows
        self.jobView.setAlternatingRowColors(True)

        # Set row selection behaviour
        self.jobView.setSelectionBehavior(QAbstractItemView.SelectRows)

        #self.jobView.resizeColumnsToContents()
        # Set the column width specifically, otherwise they are either too big or too small
        for column in range(1, 5):
            self.jobView.setColumnWidth(column, 70)

        # Hide vertical header
        vh = self.jobView.verticalHeader()
        vh.setVisible(False)

        # Enable sorting
        self.jobView.setSortingEnabled(True)

        # Set horizontal header properties
        hh = self.jobView.horizontalHeader()
        hh.setStretchLastSection(True)
        hh.setFont(font)

class SystemTrayIcon(QSystemTrayIcon):

    def __init__(self, icon, parent=None):
        QSystemTrayIcon.__init__(self, icon, parent)

if __name__ == '__main__':

    app = QApplication(sys.argv)

    app.setStyleSheet("QStatusBar::item { border: 0px solid black }; "); 

    # Set up Stone
    initConfig('db.ini', '/tmp/farmstats.log')
    initStone(sys.argv)
    classes_loader()
    Database.current().connection().reconnect()

    main = Afterburner()
    main.show()

    sys.exit(app.exec_())
