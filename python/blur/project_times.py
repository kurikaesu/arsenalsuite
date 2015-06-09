#!/usr/bin/python

from blur.Stone import *
from blur.Classes import *
from PyQt4.QtCore import *
#from PyQt4.QtGui import *
#from PyQt4.uic import Loader
import sys

app = QCoreApplication(sys.argv)

initConfig( "/etc/db.ini" );
blurqt_loader()
Database.instance().setEchoMode( Database.EchoUpdate | Database.EchoDelete | Database.EchoSelect )

pj = ProjectStatus.recordByName( "Production" )
pl = Project.select().filter( "fkeyprojectstatus", QVariant(pj.key()) )

def updateDates( el, date, project ):
	for i in range(0,el.size()):
		e = el[i]
		if not e.startDate().isValid() or e.startDate() < date:
			e.setStartDate( date )
		if not e.dateComplete().isValid() or e.dateComplete() < e.startDate():
			e.setDateComplete( e.startDate().addDays(1) )
		if not e.project().isRecord():
			e.setProject( project )
		e.commit()
		updateDates( e.children(), e.startDate(), project )

for i in range( 0, pl.size() ):
	p = pl[i]
	if not p.dateComplete().isValid() or p.startDate() > p.dateComplete():
		p.setDateComplete( p.startDate().addDays( 1 ) )
		p.commit()
	updateDates( p.children(), p.startDate(), p )