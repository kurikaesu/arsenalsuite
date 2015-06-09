
from blur.quickinit import *

def updateUsersLastLogon():
	usr = User.currentUser()
	usr.setColumnLiteral( 'dateoflastlogon', 'now()::date' )
	usr.setLastLogonType( 'local' )
	usr.setLogonCount( usr.logonCount() + 1 )
	usr.commit()
	
if __name__=='__main__':
	updateUsersLastLogon()
	
