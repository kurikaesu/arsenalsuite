
from PyQt4.QtCore import *
from pyExcelerator import *
from pyExcelerator.Utils import *
from blur.Stone import *
from blur.Classes import *
import re
import sys
from defaultdict import DefaultDict

class Report:
	def __init__(self,Name,Title,Function):
		self.Name = Name
		self.Title = Title
		self.Function = Function

# Contains all report functions, Can contain null(None)
# entries, those indicate a placeholder for menu separators
ReportList = []

####################################
# Utility Functions
####################################
def write_vals(worksheet,row,col,vals):
# Utility function for writing consecutive values
	for v in vals:
		if isinstance(v,QString):
			v = unicode(v)
		worksheet.write(row,col,v)
		col += 1

def sum_str(begin, end):
# Returns SUM formula, given the begin and end range
# Specified as two tuples containing row,col
	r1,c1 = begin
	r2,c2 = end
	return "SUM(" + rowcol_to_cell(r1,c1) + ":" + rowcol_to_cell(r2,c2) + ")"

def set_col_width( worksheet, col, width ):
	worksheet.col(col).width = width * 37

def set_col_widths( worksheet, col_widths ):
	for col, width in col_widths.iteritems():
		set_col_width( worksheet, col, width )

def set_row_height( worksheet, row, height ):
	worksheet.row(row).height = height * 37

def style_title():
	font = Font()
	font.bold = True
	font.colour_index = 0x1
	font.name = 'Lucida Sans Unicode'
	font.height = 300 
	
	pat = Pattern()
	pat.pattern = Pattern.SOLID_PATTERN
	pat.pattern_fore_colour = 0x32

	align = Alignment()
	align.horz = Alignment.HORZ_CENTER
	
	borders = Borders()
	borders.left = 0
	borders.right = 0
	borders.top = 3
	borders.bottom = 0
	
	style = XFStyle()
	style.font = font
	style.pattern = pat
	style.alignment = align
	style.borders = borders
	return style

def style_header():
	font = Font()
	font.bold = True
	font.colour_index = 0x1 
	font.name = 'Lucida Sans Unicode'
	font.height = 200 
	
	pat = Pattern()
	pat.pattern = Pattern.SOLID_PATTERN
	pat.pattern_fore_colour = 0x32

	align = Alignment()
	align.horz = Alignment.HORZ_CENTER

	borders = Borders()
	borders.left = 0
	borders.right = 0
	borders.top = 0
	borders.bottom = 3

	style = XFStyle()
	style.font = font
	style.pattern = pat
	style.alignment = align
	style.borders = borders
	return style

def style_body():
	font = Font()
	font.bold = True
	#font.colour_index = 0x1 
	font.name = 'Lucida Sans Unicode'
	#font.height = 200 
	
	#pat = Pattern()
	#pat.pattern = Pattern.SOLID_PATTERN
	#pat.pattern_fore_colour = 0x32

	#align = Alignment()
	#align.horz = Alignment.HORZ_CENTER

	#borders = Borders()
	#borders.left = 0
	#borders.right = 0
	#borders.top = 0
	#borders.bottom = 3

	style = XFStyle()
	style.font = font
	#style.pattern = pat
	#style.alignment = align
	#style.borders = borders
	return style

#####################################
#reports
#####################################
def report_missing( datestart, dateend, timesheets, recipients = [], filename="/tmp/" + QDateTime.currentDateTime().toString() + "_timeSheetReportMissing.xls" ):
	date = datestart
	dst = datestart.toString('MM/dd/yy')
	det = dateend.toString('MM/dd/yy')
	missing = DefaultDict()
	dates_by_user = DefaultDict(list)
	
	# Gather by user and date
	for ts in timesheets:
		e = ts.user()
		d = ts.dateTime().date()
		dates_by_user[e].append(d)
	
	# Get all the employees that should be entering timesheets
	emps = Employee.select().filter("disabled",QVariant(0))
	
	# Function to sort them by name
	def emp_sort(a,b):
		if a == b:
			return 0
		if a.displayName().toUpper() > b.displayName().toUpper():
			return 1
		return -1
	
	workbook = Workbook()
	worksheet = workbook.add_sheet("Missing")
	
	date_col = 1
	user_col = 2
	
	row = 3
	col = 0
	col_init = 2

	virgin = True

	# insert Blur logo
	# worksheet.insert_bitmap('K:/Production/blur_bugs/blur_head_logo.bmp', 0, 0, 100, 100, 1, 1)
	
	# Set the column sizes
	set_col_width(worksheet,0,200)
	set_col_width(worksheet,date_col,200)
	set_col_width(worksheet,user_col,200)

	#Setup the styles
	styleTitle = style_title()
	styleHead = style_header()
	styleBody = style_body()

	#set this worksheet as the active worksheet
	worksheet.set_selected(True)

	#spread sheet titles
	worksheet.write(row, 4, "" , styleTitle)
	worksheet.write(row, 3, "" , styleTitle)
	worksheet.write(row, 2, "" , styleTitle)
	worksheet.write(row, 1, "Missing Time Sheets By Date: %s to %s" % (datestart.toString('MM/dd/yy'), dateend.toString('MM/dd/yy')), styleTitle)

	row+=1

	#spread sheet col headers
	worksheet.write(row, date_col, 	'Date',		styleHead)
	worksheet.write(row, user_col, 	'User',		styleHead)
	worksheet.write(row, user_col+1,'',		    styleHead)
	worksheet.write(row, user_col+2,'',		    styleHead)
	
	row+=1
	
	# Create email output
	while date < dateend:
		ds = date.toString('MM/dd/yy')
		if date.dayOfWeek() <= 5:
			write_vals(worksheet,row,date_col,[ds])
			row += 1
			miss = []
			for e in emps:
				if not dates_by_user.has_key(e) or not dates_by_user[e].count(date):
					miss.append(e)
					missing[e] += 1
			for e in sorted(miss,emp_sort):
				write_vals(worksheet,row,user_col,[e.displayName()])
				row += 1
		date = date.addDays(1)
		
	row += 2
	
	worksheet.write(row, 4,"", styleTitle)
	worksheet.write(row, 3,"", styleTitle)
	worksheet.write(row, 2,"", styleTitle)
	worksheet.write(row, 1,"Missing TimeSheet Count : %s to %s" % (datestart.toString('MM/dd/yy'), dateend.toString('MM/dd/yy')), styleTitle)
	
	row += 1
	
	worksheet.write(row, user_col-1,'',		     styleHead)
	worksheet.write(row, user_col, 	'User',		 styleHead)
	worksheet.write(row, user_col+1,'Missing', styleHead)
	worksheet.write(row, user_col+2,'',        styleHead)
	
	row += 1
	
	for e in sorted(missing.keys(),emp_sort):
		write_vals(worksheet,row,user_col,[e.displayName(),missing[e]])
		row += 1
		
	workbook.save(filename)
	body = "Attached is a missing time sheets summary for " + datestart.toString('MM/dd/yy') + " - " + dateend.toString('MM/dd/yy') + "\n\nthePipe\n"
	return email_report( recipients, "Missing timesheets: " + dst + " - " + det, body, SS=filename )

def report_category( dateStart, dateEnd, timesheets, recipients, filename="/tmp/" + QDateTime.currentDateTime().toString() + "_timeSheetCatSum.xls" ):
	workbook = Workbook()
	worksheet = workbook.add_sheet("Category-Project")

	proj_col = 2
	type_col = 1
	hr_act_col = 3
	sum_col = 4

	row = 3
	col = 0
	col_init = 2

	virgin = True

	project_dict = DefaultDict( DefaultDict( list ) )

	for ts in timesheets:
		project_dict[ts.project()][ts.assetType()].append(ts)

	# insert Blur logo
	# worksheet.insert_bitmap('K:/Production/blur_bugs/blur_head_logo.bmp', 0, 0, 100, 100, 1, 1)

	# Set the column sizes
	set_col_width(worksheet,0,200)
	set_col_width(worksheet,proj_col,200)
	set_col_width(worksheet,type_col,200)
	set_col_width(worksheet,hr_act_col,150)

	#Setup the styles
	styleTitle = style_title()
	styleHead = style_header()
	styleBody = style_body()

	#set this worksheet as the active worksheet
	worksheet.set_selected(True)

	#spread sheet titles
	worksheet.write(row, proj_col+4, "", styleTitle)
	worksheet.write(row, proj_col+3, "", styleTitle)
	worksheet.write(row, proj_col+2, "", styleTitle)
	worksheet.write(row, proj_col+1, "", styleTitle)
	worksheet.write(row, proj_col  , "", styleTitle)
	worksheet.write(row, proj_col-1, "Time Sheet Summary by Category and Project: %s - %s" % (dateStart.toString('MM/dd/yy'), dateEnd.toString('MM/dd/yy')), styleTitle)

	row+=1

	#spread sheet col headers
	worksheet.write(row, proj_col, 	'Projects',		  styleHead)
	worksheet.write(row, type_col, 	'Category',		  styleHead)
	worksheet.write(row, hr_act_col,'Actual Hours',	styleHead)
	worksheet.write(row, hr_act_col+1,'',	styleHead)
	worksheet.write(row, hr_act_col+2,'',	styleHead)
	worksheet.write(row, hr_act_col+3,'',	styleHead)

	row+=1
	
	
	# Loop through each project
	hrs = 0
	def psort(a,b):
		if a == b:
			return 0
		if a.name().toUpper() > b.name().toUpper():
			return 1
		return -1
	
	for p in sorted(project_dict.keys(),psort):
		at_dict = project_dict[p]
		for at, sheets in at_dict.iteritems():
			# TODO, change to sort using chronology
			hrs = 0
			for ts in sheets:
				hrs += ts.scheduledHour()
				# Write a value for each asset type
				if at != ts.assetType():
					write_vals(worksheet,row,1,[at.name(),p.name(),hrs])
					#worksheet.write(row,sum_col,Formula("SUM(D10:D15)")) #not working wtf ...drp
					row += 1
			# Write the last one
			write_vals(worksheet,row,1,[at.name(),p.name(),hrs])
			row += 1

	workbook.save(filename)
	# Send email
	body = "Attached is a time sheet summary by category and project for " + dateStart.toString('MM/dd/yy') + " - " + dateEnd.toString('MM/dd/yy') + "\n\nthePipe\n"
	return email_report( recipients, subject="Time sheet summary by category and project: %s - %s" % (dateStart.toString('MM/dd/yy'), dateEnd.toString('MM/dd/yy')), body=body, SS=filename );


def report_prj_cat_emp( dateStart, dateEnd, timesheets, recipients, filename='/tmp/' + QDateTime.currentDateTime().toString() + '_timeSheetProjectSum.xls' ):
	#new spread sheet
	workbook = Workbook()
	worksheet = workbook.add_sheet( "Project-Category-Employee" )
	
	style = style_title()
	type = AssetType()
	emp = Employee()
	prj = Project()
	
	hrs = 0
	cat_hrs = 0
	prj_hrs = 0

	proj_col = 1
	type_col = 2
	empl_col = 3
	hr_act_col = 4
	cat_sum_col = 5
	prj_sum_col = 6

	row = 3
	sum_row = 3
	col = 0
	col_init= 2
	ttl_sch = 0
	
	def ts_sort(a,b):
		if a.project()!=b.project():
			return a.project().name()>b.project().name()
		if	a.assetType()!=b.assetType():
			return a.assetType().name()>b.assetType().name()
		return a.user().displayName() > b.user().displayName()

	def ts_cmp(a,b):
		if a == b:
			return 0
		bo = ts_sort(a,b)
		if bo:
			return 1
		else:
			return -1
	
	timesheets = map( lambda x: x, timesheets )
	timesheets.sort( ts_cmp )
	
	virgin = True
	for ts in timesheets:
		if virgin:
			virgin = False

			prj = ts.project()
			type = ts.assetType()
			emp = ts.user()

			# insert Blur logo
			# worksheet.insert_bitmap('K:/Production/blur_bugs/blur_head_logo.bmp', 0, 0, 100, 100, 1, 1)
			
			#Setup the styles
			styleTitle = style_title()
			styleHead = style_header()
			styleBody = style_body()
			
			#set col widths
			set_col_width(worksheet,0,200)
			set_col_width(worksheet,proj_col,200)
			set_col_width(worksheet,type_col,200)
			set_col_width(worksheet,empl_col,200)
			set_col_width(worksheet,hr_act_col,100)
			set_col_width(worksheet,cat_sum_col,100)
			set_col_width(worksheet,prj_sum_col,100)
			

			#set this worksheet as the active worksheet
			worksheet.set_selected(True)

			#spread sheet titles
			worksheet.write(row, 6, "" , styleTitle)
			worksheet.write(row, 5, "" , styleTitle)
			worksheet.write(row, 4, "" , styleTitle)
			worksheet.write(row, 3, "" , styleTitle)
			worksheet.write(row, 2, "" , styleTitle)
			worksheet.write(row, 1, "Time Sheet Summary by Project, Category and Employee: %s - %s" % (dateStart.toString('MM/dd/yy'), dateEnd.toString('MM/dd/yy')),styleTitle)

			row += 1

			#spread sheet col headers
			worksheet.write(row, proj_col, 'Project',styleHead)
			worksheet.write(row, type_col, 'Category',styleHead)
			worksheet.write(row, empl_col, 'Employee',styleHead)
			worksheet.write(row, hr_act_col, 'Emp Hours',styleHead)
			worksheet.write(row, cat_sum_col, 'Cat Hours',styleHead)
			worksheet.write(row, prj_sum_col, 'Prj Hours',styleHead)
			
			row += 1

		if prj != ts.project() or type != ts.assetType() or emp != ts.user():
			#write data to spread sheet
			write_vals(worksheet,row,1,[prj.displayName(),type.name(),emp.displayName(),ttl_sch])
			if type != ts.assetType():
				write_vals(worksheet,row,cat_sum_col,[cat_hrs])
				cat_hrs = 0
			if prj != ts.project():
				write_vals(worksheet,row,prj_sum_col,[prj_hrs])
				#worksheet.write(row,sum_col,Formula("SUM(E10:E15)")) #messing with formulas, newellm suggests "SUM(%s:%s)" % (rowcolstart,rowcolend)
				prj_hrs = 0
			row += 1
			#new project: ergo write out totals and reset ttl vars
			ttl_sch = 0

		prj = ts.project()
		type = ts.assetType()
		emp = ts.user()

		#running total
		ttl_sch += ts.scheduledHour()
		prj_hrs += ts.scheduledHour()
		cat_hrs += ts.scheduledHour()

	#write data to spread sheet
	col = 1;
	write_vals(worksheet,row,1,[prj.displayName(),type.name(),emp.displayName(),ttl_sch,cat_hrs,prj_hrs])

	workbook.save(filename)
	
	body = "Attached is a time sheet summary by project, category, and employee for " + dateStart.toString('MM/dd/yy') + " - " + dateEnd.toString('MM/dd/yy') + "\n\nthePipe\n"
	return email_report( recipients, subject="Time sheet project summary: %s - %s" % (dateStart.toString('MM/dd/yy'),dateEnd.toString('MM/dd/yy')), body=body, SS=filename)

def report_emp_cat_prj( dateStart, dateEnd, timesheets, recipients, filename='/tmp/' + QDateTime.currentDateTime().toString() + '_timeSheetProjectSum.xls' ):
	#new spread sheet
	workbook = Workbook()
	worksheet = workbook.add_sheet( "Employee-Category-Project" )
	
	style = style_title()
	type = AssetType()
	emp = Employee()
	prj = Project()
	
	hrs = 0
	cat_hrs = 0
	prj_hrs = 0

	proj_col = 3
	type_col = 2
	empl_col = 1
	hr_act_col = 6
	cat_sum_col = 5
	prj_sum_col = 4

	row = 3
	sum_row = 3
	col = 0
	col_init= 2
	ttl_sch = 0
	
	def ts_sort(a,b):
		if a.user()!=b.user():
			return a.user().displayName() > b.user().displayName()
		if	a.assetType()!=b.assetType():
			return a.assetType().name()>b.assetType().name()
		return a.project().name()>b.project().name()

	def ts_cmp(a,b):
		if a == b:
			return 0
		bo = ts_sort(a,b)
		if bo:
			return 1
		else:
			return -1
	
	timesheets = map( lambda x: x, timesheets )
	timesheets.sort( ts_cmp )
	
	virgin = True
	for ts in timesheets:
		if virgin:
			virgin = False

			prj = ts.project()
			type = ts.assetType()
			emp = ts.user()

			# insert Blur logo
			# worksheet.insert_bitmap('K:/Production/blur_bugs/blur_head_logo.bmp', 0, 0, 100, 100, 1, 1)
			
			#Setup the styles
			styleTitle = style_title()
			styleHead = style_header()
			styleBody = style_body()
			
			#set col widths
			set_col_width(worksheet,0,200)
			set_col_width(worksheet,proj_col,200)
			set_col_width(worksheet,type_col,200)
			set_col_width(worksheet,empl_col,200)
			set_col_width(worksheet,hr_act_col,100)
			set_col_width(worksheet,cat_sum_col,100)
			set_col_width(worksheet,prj_sum_col,100)
			
			#set this worksheet as the active worksheet
			worksheet.set_selected(True)

			#spread sheet titles
			worksheet.write(row, 6, "" , styleTitle)
			worksheet.write(row, 5, "" , styleTitle)
			worksheet.write(row, 4, "" , styleTitle)
			worksheet.write(row, 3, "" , styleTitle)
			worksheet.write(row, 2, "" , styleTitle)
			worksheet.write(row, 1, "Time Sheet Summary by Employee, Category, and Project: %s - %s" % (dateStart.toString('MM/dd/yy'), dateEnd.toString('MM/dd/yy')),styleTitle)

			row += 1

			#spread sheet col headers
			worksheet.write(row, proj_col, 'Project',styleHead)
			worksheet.write(row, type_col, 'Category',styleHead)
			worksheet.write(row, empl_col, 'Employee',styleHead)
			worksheet.write(row, hr_act_col, 'Emp Hours',styleHead)
			worksheet.write(row, cat_sum_col, 'Cat Hours',styleHead)
			worksheet.write(row, prj_sum_col, 'Prj Hours',styleHead)
			
			row += 1

		if prj != ts.project() or type != ts.assetType() or emp != ts.user():
			#write data to spread sheet
			write_vals(worksheet,row,1,[emp.displayName(),type.name(),prj.displayName(),prj_hrs])
			if type != ts.assetType():
				write_vals(worksheet,row,cat_sum_col,[cat_hrs])
				cat_hrs = 0
			if emp != ts.user():
				write_vals(worksheet,row,hr_act_col,[ttl_sch])
				#worksheet.write(row,sum_col,Formula("SUM(E10:E15)")) #messing with formulas, newellm suggests "SUM(%s:%s)" % (rowcolstart,rowcolend)
				ttl_sch = 0
			row += 1
			#new project: ergo write out totals and reset ttl vars
			prj_hrs = 0

		prj = ts.project()
		type = ts.assetType()
		emp = ts.user()

		#running total
		ttl_sch += ts.scheduledHour()
		prj_hrs += ts.scheduledHour()
		cat_hrs += ts.scheduledHour()

	#write data to spread sheet
	col = 1;
	write_vals(worksheet,row,1,[emp.displayName(),type.name(),prj.displayName(),prj_hrs,cat_hrs,ttl_sch])

	workbook.save(filename)
	
	body = "Attached is a time sheet summary by employee, category, and project for " + dateStart.toString('MM/dd/yy') + " - " + dateEnd.toString('MM/dd/yy') + "\n\nthePipe\n"
	return email_report( recipients, subject="Time sheet employee summary: %s - %s" % (dateStart.toString('MM/dd/yy'),dateEnd.toString('MM/dd/yy')), body=body, SS=filename)

def report_animator_days( dateStart, dateEnd, timesheets, recipients, filename='/tmp/' + QDateTime.currentDateTime().toString() + '_timeSheetAniDays.xls' ):
	EMP = {}
	for ts in timesheets:
		e = ts.user()
		if not EMP.has_key(e):
			EMP[e] = e.displayName().replace(" ","").left(12)
	
	#new spread sheet
	workbook = Workbook()
	
	#hashes need for temp and grand total calcs
	cells = {}
	grandTTLs = {}

	#create worksheets and hashes to track 'em
	totals = 'Totals';
	worksheet = {}
	virgins = {}
	rows = {}
	cols = {}

	# Create the worksheets
	worksheet[totals] = workbook.add_sheet(totals)
	def emp_sort(a,b):
		if a == b:
			return 0
		if a.displayName().toUpper() > b.displayName().toUpper():
			return 1
		return -1
	
	for e in sorted(EMP.keys(),emp_sort):
		worksheet[e] = workbook.add_sheet(unicode(EMP[e]))
		virgins[e] = True

	style = style_title()
	prj = Project()
	emp = Employee()
	dow = ''
	date = QDate()
	hours = 0
	sum_hr = 0
	ratio = 0

	empl_col 	= 1
	dow_col 	= 2
	date_col 	= 3
	proj_col 	= 4
	hour_col 	= 5
	sum_col 	= 6
	ratio_col 	= 7

	row = 3
	col = 0
	col_init = 2

	loopCount = 0

	def ts_sort(a,b):
		if a == b:
			return 0
		if a.user() != b.user():
			if a.user().displayName().toUpper() > b.user().displayName().toUpper():
				return 1
			return -1
		if a.dateTime() != b.dateTime():
			if a.dateTime() > b.dateTime():
				return 1
			return -1
		if a.project().name().toUpper() > b.project().name().toUpper():
			return 1
		return -1
	
	timesheets = sorted(timesheets,ts_sort)

	for ts in timesheets:
		e = ts.user()
		ws = worksheet[e]

		if virgins[e]:
			virgins[e] = False
			rows[e] = row
			
			#this user is virgin... but i need to write out previous employee info ...drp
			#in other words this is the last entry for the previous employee
			if loopCount:

				e = prev.user()
				cells[(rows[e],hour_col)] = {
					'project' : prj.displayName(),
					'animator' : e.displayName(),
					'hours' : hours }

				write_vals(ws,rows[e],1,[emp.displayName(),dow,date,prj.displayName(),hours])

#				if dateChange:
				cell_ttl = 0
				for (c,v) in cells.iteritems():
					cell_ttl += v['hours']
				virgin = True
				
				for c in cells.keys():
					(row, col) = c
					v = cells[c]
					if virgin:
						virgin = False
						worksheet[e].write(row, col+1, cell_ttl);
					worksheet[e].write(row, col+2, v['hours']/cell_ttl)
					del cells[c]
				e = ts.user()

			loopCount += 1
			emp = ts.user()
			prj = ts.project() #$k->project->name . ' (' . $k->fkeyproject . ')';
			date = ts.dateTime().date().toString('MM/dd/yy')
			dow = QDate.longDayName( ts.dateTime().date().dayOfWeek() )
			hours = ts.scheduledHour()
			sum_hr = ''
			ratio = '' #'=SUM(F4:F5)';
#			$hours     = ($timeSheetREF->{$k}{scheduledHour} + timeSheetREF->{$k}{unscheduledHour});

			set_col_width(ws,0,200)
			set_col_width(ws,empl_col,200)
			set_col_width(ws,dow_col,80)
			set_col_width(ws,date_col,150)
			set_col_width(ws,proj_col,200)
			set_col_width(ws,hour_col,100)
			set_col_width(ws,sum_col,100)
			set_col_width(ws,ratio_col,100)

			#set this worksheet as the active worksheet
			ws.set_selected(True)

			#spread sheet col headers
			ws.write(row-2, empl_col, 'Employee',	style)
			ws.write(row-2, dow_col,  'Day',		style)
			ws.write(row-2, date_col, 'Date',		style)
			ws.write(row-2, proj_col, 'Project',	style)
			ws.write(row-2, hour_col, 'Hours',		style)
			ws.write(row-2, sum_col,  'Ttl Hrs',	style)
			ws.write(row-2, ratio_col,'Ani Days',	style)

		else:
			dateChange = date != ts.dateTime().date().toString('MM/dd/yy')

			if emp != ts.user() or prj != ts.project() or dateChange:
				cells[(rows[e],hour_col)] = {
					'project'	:	prj.name(),
					'animator'	:	e.displayName(),
					#date	:	$date,
					'hours'	:  hours }

				write_vals(ws,rows[e],1,[emp.displayName(),dow,date,prj.displayName(),hours])
				if dateChange:
					rows[e] += 2
				else:
					rows[e] += 1

				#reset ttls
				hours = 0

				#write out kara's day percentages here ...drp
				if dateChange:
					cell_ttl = 0
					for v in cells.itervalues():
						cell_ttl += v['hours']
					virgin = True
					for c in cells.keys():
						(row, col) = c
						v = cells[c]
						if virgin:
							virgin = False
							ws.write(row,col+1,cell_ttl)
						ratio = 0
						if cell_ttl:
							ratio = v['hours'] / cell_ttl
						ws.write(row, col+2, ratio)
						if not grandTTLs.has_key( v['project'] ):
							grandTTLs[v['project']] = {}
						gttlp = grandTTLs[v['project']]
						if not gttlp.has_key( v['animator'] ):
							gttlp[v['animator']] = 0
						gttlp[v['animator']] += ratio
						del cells[c]
			emp = ts.user()
			prj = ts.project()
			date= ts.dateTime().date().toString('MM/dd/yy')
			dow = QDate.longDayName( ts.dateTime().date().dayOfWeek() )
			#running total
			hours += ts.scheduledHour()
		prev = ts

	e = prev.user()
	cells[(rows[e],hour_col)] = {
		'project' : prj.displayName(),
		'animator' : e.displayName(),
		#date		=>	$date,
		'hours' : hours }

	write_vals( worksheet[e], rows[e], 1, [emp.displayName(),dow, date, prj.displayName(), hours] )

	cell_ttl = 0
	for v in cells.itervalues():
		cell_ttl += v['hours']

	virgin = True
	for c in sorted(cells.keys()):
		(row,col) = c
		v = cells[c]
		if virgin:
			virgin = False
			worksheet[e].write(row, col+1, cell_ttl)
		worksheet[e].write(row,col+2,v['hours']/cell_ttl)
		del cells[c]

	# write project/animator totals sheet 
	worksheet[totals].set_selected(True)
	prj_row = {}
	ani_col = {}
	r = row
	for p in sorted(grandTTLs.keys()):
		prj_row[p] = r
		r += 1
		worksheet[totals].write(prj_row[p], col_init-2, unicode(p), style )
		for e in grandTTLs[p].keys():
			ani_col[e] = 1

	c = col_init
	for e in sorted(ani_col.keys()):
		ani_col[e] = c
		c += 1
		worksheet[totals].write(row-2, ani_col[e], unicode(e), style)

	for i in range(row,r):
		worksheet[totals].write(i, c, sum_str( (i,col_init), (i,c-1) ) )
		
	for i in range(col_init,c):
		worksheet[totals].write(r, i, sum_str((row,i),(r-1,i)))
	
	for p in sorted(grandTTLs.keys()):
		for e in sorted(grandTTLs[p].keys()):
			worksheet[totals].write(prj_row[p], ani_col[e], grandTTLs[p][e])

	workbook.save(filename)

	body = "Attached is the month ending time sheet.\n\nthePipe\n"
	return email_report( recipients, "Time sheet animator days: %s - %s" % (dateStart.toString('MM/dd/yy'),dateEnd.toString('MM/dd/yy')), body, SS=filename )


def report_averages( dateStart, dateEnd, timesheets, recipients, filename='/tmp/%s_timeSheetAverages.xls' % QDateTime.currentDateTime().toString() ):
	class EmployeeSummary:
		def __init__(self,employee,dateStart,dateEnd):
			self.employee = employee
			self.total_hours = 0
			self.total_posts = 0
			
			self.hours_workday = 0
			self.workday_by_date = {}
			self.posts_workday = 0
			
			self.hours_weekend = 0
			self.weekend_by_date = {}
			self.posts_weekend = 0
			
			self.hours_holiday = 0
			self.holiday_by_date = {}
			self.posts_holiday = 0
			self.holiday_count = 0
			
			self.hours_vacation = 0
			self.vacation_by_date = {}
			self.posts_vacation = 0
			
		def __cmp__(self,other):
			if self.employee.displayName().toUpper() > other.employee.displayName().toUpper():
				return 1
			return -1
			
		def is_vacation(self,date):
			return date in self.vacation_by_date
		
		def is_holiday(self,date):
			jan_1 = QDate(date.year(),1,1)
			sep_1 = QDate(date.year(),9,1)
			oct_1 = QDate(date.year(),10,1)
			nov_1 = QDate(date.year(),11,1)
			
			# Memorial day is last friday in may
			mem_day = QDate(date.year(),5,1)
			mem_day = mem_day.addDays((mem_day.dayOfWeek()-1)%7)
			while mem_day.addDays(7).month() == 5:
				mem_day = mem_day.addDays(7) 
			
			return date in [
				QDate(date.year(),1,1),  							# New Years Day
				jan_1.addDays((jan_1.dayOfWeek()-1)%7 + 14), 		# Martin Luther King, 3rd Monday of Jan
				QDate(date.year(),7,4),								# Independance day, 4th of july
				sep_1.addDays((sep_1.dayOfWeek()-1)%7),				# Labor Day, first Monday in Sept
				oct_1.addDays((oct_1.dayOfWeek()-1)%7+7),			# Columbus Day, second Monday in Oct
				nov_1.addDays((nov_1.dayOfWeek()-4)%7+21),			# Thanksgiving, 4th thursday in Nov
				QDate(date.year(),12,25), 							# Christmas
			]
	
		# Returns 1 if date is a valid work day, else 0
		def is_valid_workday(self,date):
			if date.dayOfWeek() <= 5 and not self.is_holiday(date) and not self.is_vacation(date):
				return True
			return False
		
		# Assumes days_vacation and days_holiday are populated
		def calc_day_counts(self,dateStart,dateEnd):
			if self.employee.dateOfHire().isValid() and self.employee.dateOfHire() > dateStart:
				dateStart = self.employee.dateOfHire()
			currentDate = QDate.currentDate()
			if dateEnd > currentDate:
				dateEnd = currentDate
			self.workday_count = 0
			while dateStart <= dateEnd:
				if self.is_valid_workday(dateStart):
					self.workday_count += 1
				if self.is_holiday(dateStart):
					self.holiday_count += 1
				dateStart = dateStart.addDays(1)
			self.vacationday_count = len(self.vacation_by_date)
			self.workedday_count = len(self.workday_by_date)
			self.weekend_count = len(self.weekend_by_date)
			
		def add(self,timesheet):
			hrs = timesheet.scheduledHour()
			dt = ts.dateTime()
			ds = dt.date().toString()
			weekend = dt.date().dayOfWeek() > 5
						
			#sum hours for report range
			self.total_hours += hrs
			self.total_posts += 1
			
			if weekend:
				self.hours_weekend += hrs
				self.posts_weekend += hrs
				self.weekend_by_date[ds] = True
				
			elif timesheet.assetType().name() == 'Vacation':
				self.hours_vacation += hrs
				self.posts_vacation += 1
				self.vacation_by_date[ds] = True
			
			elif self.is_holiday(timesheet.dateTime().date()):
				self.hours_holiday += hrs
				self.posts_holiday += 1
				self.holiday_by_date[ds] = True
				
			else:
				self.hours_workday += hrs
				self.posts_workday += 1
				self.workday_by_date[ds] = True
			
		def calc_averages(self):
			def zdiv(a,b):
				if b:
					return a/float(b)
				return 0
			
			self.average_total_hours_per_worked_day = zdiv(self.total_hours, self.workedday_count)
			self.average_total_hours_per_workday = zdiv(self.total_hours, self.workday_count)
			self.average_workday_hours_per_worked_day = zdiv(self.total_hours, self.workedday_count)
			self.average_weekend_hours_per_weekend_worked = zdiv(self.hours_weekend, self.weekend_count)
			self.average_holiday_hours_per_holiday_worked = zdiv(self.hours_holiday, self.holiday_count)
			
			self.working_days_without_posts = self.workday_count - self.workedday_count
			self.working_day_post_perc = int(100.0 * zdiv(self.workedday_count,self.workday_count))
			
	employee_summaries = {}
	
	#iterate over records: counting posts, totaling hours, etc
	for ts in timesheets:
		if not ts.user() in employee_summaries:
			employee_summaries[ts.user()] = EmployeeSummary(ts.user(), dateStart, dateEnd)
		employee_summaries[ts.user()].add(ts)
		 
	for e in employee_summaries:
		employee_summaries[e].calc_day_counts(dateStart,dateEnd)
		employee_summaries[e].calc_averages()
		
		#new spread sheet
	workbook = Workbook()
	style = style_title()

	empl_col 			= 1
	posts_col 			= 2
	possible_posts_col 	= 3
	post_percentage 	= 4
	ttl_hrs_col 		= 5
	avg_weekday_hrs_col = 6
	avg_weekend_hrs_col = 7 
	avg_hrs_col 		= 8
	
	row	= 3
	col = 1

	#create worksheets and hashes to track 'em
	totals = 'Averages'
	worksheet = workbook.add_sheet(totals)

	# insert Blur logo
	# worksheet.insert_bitmap('K:/Production/blur_bugs/blur_head_logo.bmp', 0, 0, 100, 100, 1, 1)
			
	#Setup the styles
	styleTitle = style_title()
	styleHead = style_header()
	styleBody = style_body()
	
	#set col widths
	set_col_width(worksheet,0,200)
	set_col_width(worksheet,empl_col,180)
	set_col_width(worksheet,posts_col,100)
	set_col_width(worksheet,possible_posts_col,100)
	set_col_width(worksheet,post_percentage,50)
	set_col_width(worksheet,ttl_hrs_col,100)
	set_col_width(worksheet,avg_weekday_hrs_col,100)
	set_col_width(worksheet,avg_weekend_hrs_col,100)
	set_col_width(worksheet,avg_hrs_col,100)

	#set this worksheet as the active worksheet
	worksheet.set_selected(True)

	#spread sheet titles
	worksheet.write(row, 9, "", styleTitle)
	worksheet.write(row, 8, "", styleTitle)
	worksheet.write(row, 7, "", styleTitle)
	worksheet.write(row, 6, "", styleTitle)
	worksheet.write(row, 5, "", styleTitle)
	worksheet.write(row, 4, "", styleTitle)
	worksheet.write(row, 3, "", styleTitle)
	worksheet.write(row, 2, "", styleTitle)
	worksheet.write(row, 1, "Time Sheet Averages by Employee %s - %s" % (dateStart.toString('MM/dd/yy'), dateEnd.toString('MM/dd/yy')), styleTitle)

	row += 1
	
	#spread sheet col headers
	worksheet.write(row, empl_col, 				'Employee',		 	styleHead)
	worksheet.write(row, posts_col, 			'Days posted', styleHead)
	worksheet.write(row, possible_posts_col, 	'Work Days',	styleHead)
	worksheet.write(row, post_percentage, 		'Post %',			styleHead)
	worksheet.write(row, ttl_hrs_col, 			'TTL Hrs',			styleHead)
	worksheet.write(row, avg_weekday_hrs_col, 	'Avg WorkDay Hrs',			styleHead)
	worksheet.write(row, avg_weekend_hrs_col, 	'Avg Weekend Hrs', 			styleHead)
	worksheet.write(row, avg_hrs_col, 			'Per Workday',			styleHead)
	worksheet.write(row, avg_hrs_col+1, 			'',			styleHead)

	row += 2

	for summary in sorted(employee_summaries.values()):
		col = 1
		write_vals( worksheet, row, 1, [
			summary.employee.displayName(),
			summary.workedday_count,
			summary.workday_count,
			summary.working_day_post_perc,
			summary.total_hours,
			summary.average_workday_hours_per_worked_day,
			summary.average_weekend_hours_per_weekend_worked,
			summary.average_total_hours_per_workday
			 ] )
		row += 1
	
	body = """
Attached is the average hours time sheet.

thePipe
"""

	workbook.save(filename)
	
	return email_report( recipients, "Time sheet averages: %s - %s" % (dateStart.toString('MM/dd/yy'), dateEnd.toString('MM/dd/yy')), body, SS=filename )


def report_dump(dateStart,dateEnd, timesheets, recipients, filename='/tmp/%s_timeSheetDump.xls' % QDateTime.currentDateTime().toString()):
	#new spread sheet
	workbook  = Workbook()
	worksheet = workbook.add_sheet("Timesheet Dump")

	#Setup the styles
	styleTitle = style_title()
	styleHead = style_header()
	styleBody = style_body()
	
	ttl_sch = 0
	HrsPerDay 	= 8

	empl_col 	= 1
	date_col 	= 2
	proj_col 	= 3
	type_col 	= 4
	asset_col 	= 5
	comment_col = 6
	hr_act_col 	= 7
	days_col 	= 8
	days_8hr_col = 9
	
	row			= 6
	col 		= 0
	col_init	= 2

	virgin 		= True
	
	def ts_cmp(a,b):
		if a.user() != b.user():
			return a.user().displayName().toUpper() > b.user().displayName().toUpper()
		if a.dateTime() != b.dateTime():
			return a.dateTime() > b.dateTime()
		if a.project() != b.project():
			return a.project().name().toUpper() > b.project().name().toUpper()
		return a.assetType().name().toUpper() > b.assetType().name().toUpper()
	def ts_sort(a,b):
		if a == b:
			return 0
		if ts_cmp(a,b):
			return 1
		return -1
	
	class DaySummary(object):
		def __init__(self):
			self.timeSheetCount = 0
			self.totalHours = 0
		def addTs(self,hours):
			self.totalHours += hours
			self.timeSheetCount += 1
		def days(self, hrs):
			Log( "Hrs %i, totalHours %i, tsCount %i" % (hrs,self.totalHours,self.timeSheetCount) )
			if self.timeSheetCount > 1 and self.totalHours > 8: #more than one time sheet entry
				return hrs / self.totalHours
			if hrs > 8:
				return 1
			else:
				return hrs / 8.0
	
	summariesByUserAndDay = DefaultDict( DefaultDict( DaySummary ) )
	
	for ts in sorted(timesheets,ts_sort):
		print 'Adding %i hrs for %s user on %s date to DaySummary %s' % (ts.scheduledHour(), ts.user().name(), ts.dateTime().date().toString(), unicode(summariesByUserAndDay[ts.user()][ts.dateTime().date().toString()]) )
		summariesByUserAndDay[ts.user()][ts.dateTime().date().toString()].addTs( ts.scheduledHour() )
			
	for ts in sorted(timesheets,ts_sort):
		if virgin:
			virgin = False

			set_col_width(worksheet,0,50)
			set_col_width(worksheet,empl_col,160)
			set_col_width(worksheet,date_col,100)
			set_col_width(worksheet,proj_col,160)
			set_col_width(worksheet,type_col,180)
			set_col_width(worksheet,asset_col,200)
			set_col_width(worksheet,hr_act_col,40)
			set_col_width(worksheet,comment_col,220)
			set_col_width(worksheet,days_col,40)
			set_col_width(worksheet,days_8hr_col,65)
			
			#set this worksheet as the active worksheet
			worksheet.set_selected(True)

			#spread sheet titles
			set_row_height(worksheet,row-3,70)
			worksheet.write(row-3, 9, "", styleTitle)
			worksheet.write(row-3, 8, "", styleTitle)
			worksheet.write(row-3, 7, "", styleTitle)
			worksheet.write(row-3, 6, "", styleTitle)
			worksheet.write(row-3, 5, "", styleTitle)
			worksheet.write(row-3, 4, "", styleTitle)
			worksheet.write(row-3, 3, "", styleTitle)
			worksheet.write(row-3, 2, "", styleTitle)
			worksheet.write(row-3, 1, "Time Sheet Dump %s - %s" % (dateStart.toString('MM/dd/yy'), dateEnd.toString('MM/dd/yy')), styleTitle)

			#spread sheet col headers
			worksheet.write(row-2, empl_col,    'Employee',	styleHead)
			worksheet.write(row-2, date_col,    'Date',    	styleHead)
			worksheet.write(row-2, proj_col,    'Projects',	styleHead)
			worksheet.write(row-2, type_col,    'Category',	styleHead)
			worksheet.write(row-2, asset_col,   'Asset',		styleHead)
			worksheet.write(row-2, comment_col, 'Comments', styleHead)
			worksheet.write(row-2, hr_act_col,  'Hours',	  styleHead)
			worksheet.write(row-2, days_col,    'Days',	    styleHead)
			worksheet.write(row-2, days_8hr_col, 'Days (8hr)', styleHead)
		
		dat = ts.dateTime().date().toString('yyyy/MM/dd ddd')
		prj = ts.project()
		type = ts.assetType()
		emp = ts.user()
		hr = ts.scheduledHour()
		
		#...drp if more than one time sheet entry for this day divid by TOT[k] else if only one entry message 
		days = 0
		if ts.dateTime().date().dayOfWeek() > 5:
			days = 0 #dont count weekends
		else:
			days = summariesByUserAndDay[ts.user()][ts.dateTime().date().toString()].days(hr)
		
		elem = ''
		if ts.element() != ts.project():
			elem = ts.element().displayName(True)
		
		write_vals( worksheet, row, 1, [emp.displayName(), dat, prj.displayName(), type.name(), elem, ts.comment(), hr, days, hr / 8.0] )
		row += 1
	
	workbook.save(filename)

	body = """
Attached is a dump of all time sheets %s - %s.

thePipe
""" % ( dateStart.toString('MM/dd/yy'), dateEnd.toString('MM/dd/yy') )

	return email_report( recipients, "Time sheet dump: %s - %s" % (dateStart.toString('MM/dd/yy'),dateEnd.toString('MM/dd/yy')), body, SS=filename )
	
def report_slave_summaries(dateStart,dateEnd, filename='/tmp/%s_timeSheetDump.xls' % QDateTime.currentDateTime().toString()):
	
	#new spread sheet
	workbook  = Workbook()
	worksheet = workbook.add_sheet("User's Host Slave Summary")

	# Get calendars for the date
	stats = Database.current().exec_( "SELECT * FROM hosthistory_user_slave_summary( ?, ? );", [QVariant(dateStart),QVariant(dateEnd)] );
	
	user_col = 1
	host_col = 2
	time_col = 3
	
	set_col_width(worksheet,0,50)
	set_col_width(worksheet,user_col,160)
	set_col_width(worksheet,host_col,100)
	set_col_width(worksheet,time_col,160)
	
	row = 2
	worksheet.write(row, 1, "User's Host Slave Summary %s - %s" % (dateStart.toString('MM/dd/yy'), dateEnd.toString('MM/dd/yy')) )
	row += 1
	
	worksheet.write(row, user_col, 'User')
	worksheet.write(row, host_col, 'Host')
	worksheet.write(row, time_col, 'Slave Time')
	row += 1
	
	while stats.next():
		worksheet.write( row, user_col, unicode(stats.value(0).toString()) )
		worksheet.write( row, host_col, unicode(stats.value(1).toString()) )
		worksheet.write( row, time_col, unicode(stats.value(2).toString()) )
		row += 1

	workbook.save(filename)

def padString(s,pl):
	if( len(s) < pl ):
		s = (' ' * (pl - len(s))) + s
	return s

def report_render_summaries(dateStart,dateEnd, filename='/tmp/%s_timeSheetDump.xls' % QDateTime.currentDateTime().toString()):
	
	#new spread sheet
	workbook  = Workbook()
	worksheet = workbook.add_sheet("User's Render Time Summaries")

	# Get calendars for the date
	stats = Database.current().exec_( "select usr.name, sum(coalesce(totaltasktime,'0'::interval)) as totalrendertime, sum(coalesce(totalerrortime,'0'::interval)) as totalerrortime, sum(coalesce(totalerrortime,'0'::interval))/sum(coalesce(totaltasktime,'0'::interval)+coalesce(totalerrortime,'0'::interval)) as errortimeperc from jobstat, usr where started > 'today'::timestamp - '7 days'::interval and ended is not null and fkeyusr=keyelement group by usr.name order by errortimeperc desc;" )
	
	(user_col, rnd_time_col, err_time_col, err_perc_col) = range(1,5)
	col_widths = {0: 50, user_col: 160, rnd_time_col: 100, err_time_col: 100, err_perc_col: 100}
	set_col_widths(worksheet, col_widths)
	
	row = 2
	worksheet.write(row, 1, "User's Render Time Summary %s - %s" % (dateStart.toString('MM/dd/yy'), dateEnd.toString('MM/dd/yy')) )
	row += 1
	
	col_names = {user_col:'User', rnd_time_col:'Render Time', err_time_col:'Error Time', err_perc_col:'Error %'}
	for col, name in col_names.iteritems():
		worksheet.write(row, col, name)
	row += 1
	
	while stats.next():
		for idx, col in enumerate( [user_col, rnd_time_col, err_time_col, err_perc_col] ):
			s = stats.value(idx).toString()
			if idx in [1,2]:
				(s,success) = Interval.reformat(s,Interval.Hours,Interval.Seconds)
			elif idx==3:
				s = '%1.2f' % float(s)
			worksheet.write( row, col, unicode(s) )
		row += 1

	workbook.save(filename)

def encrypt(args, eurikey = None):
	import blowfish.Blowfish
	pkey = 'JE87z39322aiscpqpzx94KJ29SN400mndhqn7198zfgQQAZMKLP6478A'
	if eurikey: pkey = eurikey
	cipher = Blowfish(pkey)

	# create the URI
	al = []
	for (k,v) in args.itervalues():
		al.append( k + "=" + v )
	arg_string = string.join(al,"&")

	return cipher.encrypt(arg_string)


def send_email( recipients, subject, body, attachments=[], sender = None ):
	import smtplib
	from email.MIMEText import MIMEText
	from email.MIMEMultipart import MIMEMultipart
	from email.MIMEBase import MIMEBase
	from email import Encoders
	
	if not sender:
		sender = Config.recordByName( "emailDefault" ).value()
	
	for (i,v) in enumerate(recipients):
		if not v.count('@'):
			recipients[i] = v + unicode(Config.recordByName( "emailDomain" ).value())
	
	email = MIMEMultipart(unicode(body))
	email['Subject'] = unicode(subject)
	email['From'] = unicode(sender)
	email['To'] = ', '.join(recipients)
	email['Date'] = unicode(QDateTime.currentDateTime().toString('ddd, d MMM yyyy hh:mm:ss'))
	email.preamble = 'This is a multi-part message in MIME format.'
	
	msgAlternative = MIMEMultipart('alternative')
	email.attach(msgAlternative)
	
	msgText = MIMEText(unicode(body))
	msgAlternative.attach(msgText)

	for a in attachments:
		fp = open( str(a), 'rb' )
		txt = MIMEBase("application", "octet-stream")
		txt.set_payload(fp.read())
		fp.close()
		Encoders.encode_base64(txt)
		txt.add_header('Content-Disposition', 'attachment; filename="%s"' % a)
		email.attach(txt)
		
	s = smtplib.SMTP()
	s.connect("mail.blur.com")
	print "Sending email to ", recipients
	s.sendmail(str(sender), recipients, email.as_string())
	s.close()

def emailReminder( recipients ):
	thisdate = QDate.currentDate().toString()
	link = encrypt( {'class':'Blur::Model::TimeSheet','method':'view','thisDate':thisdate})

	body = """
Please click the time sheet link below and post your hours.

http://www.blur.com/thePipe.html?e=1\&o=%s

Thanks,
thePipe
""" % link

	send_email( recipients, "Time sheet for " + QDate.currentDate().toString('MM/dd/yy'), "", body )
	print 'Blur.reports.emailReminder: Timesheet reminders sent out'


def email_report( recipients, subject, body, SS = None, attachments = [] ):
	# Check for the spreadsheet
	if SS:
		if not QFile.exists(SS):
			print "Doh! Excel spread sheet", SS, "does not exist."
			return
		attachments.append(SS)
			
	# Send the mail
	if len(recipients):
		send_email( recipients, subject, body, attachments )
		print "blur.reports.email_report:", subject + " --emailed to " + ','.join(recipients)

ReportList.append( Report( "missing",					"Missing Time Sheets by date", 							report_missing ) )
ReportList.append( Report( "dump",						"Time Sheet Dump", 										report_dump ) )
ReportList.append( None ) # Menu separator
ReportList.append( Report( "projectcategory",			"Time Sheet Summary by Category and Project", 			report_category ) )
ReportList.append( Report( "projectcategoryemployee", 	"Time Sheet Summary by Project, Category, and Employee", report_prj_cat_emp ) )
ReportList.append( Report( "employeecategoryproject",	"Time Sheet Summary by Employee, Category, and Project", report_emp_cat_prj ) )
ReportList.append( None ) # Menu separator
ReportList.append( Report( "employeeaverages", "Time Sheet Averages by Employee", report_averages ) )

if __name__ == '__main__':
	# Testing stuff
	app = QCoreApplication(sys.argv)
	initConfig("/etc/db.ini")
	blurqt_loader()
	recips = []
	start = QDate.currentDate().addDays(-7)
	end = QDate.currentDate()
	#tsl = TimeSheet.select( "dateTime >= ? AND dateTime <= ?", [QVariant(start),QVariant(end)] )
	#report_missing( start, end, tsl, recips )
	#report_category( start, end, tsl, recips )
	#report_emp_cat_prj( start, end, tsl, recips )
	#report_animator_days( start, end, tsl, recips )
	#report_averages( start, end, tsl, recips )
	#report_dump( start, end, tsl, recips )
	report_slave_summaries(start,end)
	report_render_summaries(start,end)
