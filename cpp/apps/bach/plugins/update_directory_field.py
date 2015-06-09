
import initbach

Database.current().exec_("update bachasset set directory = substring(path from '.*/');")

