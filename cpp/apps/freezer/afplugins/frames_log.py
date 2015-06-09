# All the arsenal libs that we use
from blur.Stone import *
from blur.Classes import *
from blur.Freezer import *

# PyQt libs for GUI manipulation
from PyQt4.QtCore import *
from PyQt4.QtSql import *
from PyQt4.QtGui import *

class FrameLogViewerPlugin(JobFramesTabWidgetPlugin):
    def __init__(self):
        JobFramesTabWidgetPlugin.__init__(self)
        self.wantedTask = None
        self.jobTaskAssignments = None
        self.assignedTab = 0

    def name(self):
        return QString("Frames tab log viewer")

    # Called by freezer when it starts building the frames tab on startup
    def initialize(self, parent):
        # Add a new tab
        self.tabWidget = QWidget()
        self.assignedTab = parent.addTab(self.tabWidget, "Logs")

        # Initialise the layout for the log tab
        self.verticalLayout = QVBoxLayout(self.tabWidget)
        
        # Add the QComboBox for the previous runs
        self.previousRunsDropDown = QComboBox(self.tabWidget)

        # Initialise the log viewer
        self.logviewer = QTextEdit(self.tabWidget)
        self.logviewer.setReadOnly(True)

        # Add the text editor to the layout and show the table.
        self.verticalLayout.addWidget(self.previousRunsDropDown)
        self.verticalLayout.addWidget(self.logviewer)

        # Connect the combo box to change the log view
        QObject.connect(self.previousRunsDropDown, SIGNAL('currentIndexChanged(int)'), self.setLogView)

    # Called by freezer when you click on a job
    def setJobList(self, jobList, currentIndex):
        pass

    # Called by freezer when you click on a job task
    def setJobTaskList(self, jobTasks, currentIndex):
        if currentIndex != self.assignedTab:
            return

        # If we haven't selected anything, then let's not do anything
        if jobTasks.size() == 0:
            return

        # We clear the log viewer
        self.logviewer.clear()

        # reset the job task assignments
        self.jobTaskAssignments = None

        # This is a log viewer so we only take the first task
        wantedTask = jobTasks[0]
        if wantedTask.isRecord():
            # Store the record
            self.wantedTask = wantedTask

            self.setLogView(None)
            self.getPreviousTaskRun(wantedTask)
        else:
            print "Received a task that isn't a record"

    # Generate the contents of what should be in the log file viewer
    def setLogView(self, index):
        # Grab the assignment from the task
        assignment = None
        if index != None and self.jobTaskAssignments != None:
            assignment = self.jobTaskAssignments[index].jobAssignment()
        else:
            assignment = self.wantedTask.jobTaskAssignment().jobAssignment()

        # Get the log file name
        log = assignment.stdOut()

        # Grab the text cursor
        textCursor = self.logviewer.textCursor()

        # Ensures that if you choose a new frame that hasn't run before we don't try opening anything
        if log != '':
            finalLogs = []
            file = open(log, 'r')
            line = file.readline()

            while line != '':
                finalLogs.append(line)
                line = file.readline()

            # Set the text
            self.logviewer.setText("".join(finalLogs))

        # Move the cursor position to the end of the log so that we view the bottom first
        textCursor.movePosition(QTextCursor.End)
        self.logviewer.setTextCursor(textCursor)

    # Gather the previous logs for the task
    def getPreviousTaskRun(self, jobTask):
        # This is true only when the combo box is not shown
        if jobTask == None:
            return

        # We clear the combo box otherwise it'll just append
        self.previousRunsDropDown.clear()

        # Grab the job assignments related to the task
        self.jobTaskAssignments = JobTaskAssignment.select("fkeyjobtask=? order by ended desc", [QVariant(jobTask.key())])
        for jta in self.jobTaskAssignments:
            assignment = jta.jobAssignment()
            if assignment.isRecord():
                if assignment.ended().toString() == "":
                    self.previousRunsDropDown.addItem("Currently running.. - %s" % assignment.host().name())
                else:
                    self.previousRunsDropDown.addItem("%s - %s" % (assignment.ended().toString("dd/MM/yy - hh:mm:ss"), assignment.host().name()))

# Don't forget to register the widget with the factory or it won't show up in freezer
JobFramesTabWidgetFactory.registerPlugin( FrameLogViewerPlugin() )
