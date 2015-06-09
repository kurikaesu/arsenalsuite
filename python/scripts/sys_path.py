from _winreg import *
import os, sys, win32gui, win32con

path = r'SYSTEM\CurrentControlSet\Control\Session Manager\Environment'
reg = None
key = None

def init():
	global reg, key
	reg = ConnectRegistry(None, HKEY_LOCAL_MACHINE)
	key = OpenKey(reg, path, 0, KEY_ALL_ACCESS)
	
def cleanup():
	CloseKey(key)
	CloseKey(reg)
	
def queryPath():
	value, type_id = QueryValueEx(key, 'PATH')
	return value

def show():
	init()
	print '\n'.join(queryPath().split(';'))
	cleanup()
	
def isValidPath(path):
	if len(path) == 0: return False
	if path.startswith('%') or len(path) >= 2 and path[1] == ':':
		return True
	return False

def normPath(value):
	value = value.replace('\\\\','\\')
	value = value.replace('/','\\')
	value = value.lower()
	return value

def clean(dryrun):
	init()
	path = queryPath()
	parts = path.split(';')
	print "Original Path:\n%s\n\n" % ('\n'.join(parts))
	cleaned = []
	for part in parts:
		part = normPath(part)
		if not isValidPath(part) or part in cleaned:
			continue;
		cleaned.append(part)
	path = ';'.join(cleaned)
	print "Cleaned Path:\n",'\n'.join(cleaned)
	if not dryrun:
		SetValueEx(key, 'PATH', 0, REG_EXPAND_SZ, path)
		#win32gui.SendMessage(win32con.HWND_BROADCAST, win32con.WM_SETTINGCHANGE, 0, 'Environment')
	cleanup()

def append(value):
	init()
	value = queryPath() + ';' + value
	print "Modified Path:",value
	SetValueEx(key, 'PATH', 0, REG_EXPAND_SZ, value)
	#win32gui.SendMessage(win32con.HWND_BROADCAST, win32con.WM_SETTINGCHANGE, 0, 'Environment')
	cleanup()

def remove(value):
	value = normPath(value)
	init()
	path = queryPath()
	parts = path.split(';')
	cleaned = []
	for part in parts:
		if value == normPath(part):
			print 'Found PATH to remove:',part
			continue
		cleaned.append(part)
	if len(cleaned) == len(parts):
		print 'PATH %s not found' % value
	path = ';'.join(cleaned)
	SetValueEx(key, 'PATH', 0, REG_EXPAND_SZ, path)
	win32gui.SendMessage(win32con.HWND_BROADCAST, win32con.WM_SETTINGCHANGE, 0, 'Environment')
	cleanup()
	
if __name__=='__main__':
	usage = \
"""
Usage:

sys_path.py [CMD]

CMD values
	show - DEFAULT if CMD is omitted- shows current path
	append PATH- Appends OPTION to the path
	clean - Removes duplicate and invalid entries from the path
	testclean - Same as clean, but only prints the cleaned path, without actually changing it
	remove PATH - Removes PATH from the system path if it exists
"""
	argc = len(sys.argv)
	if argc == 1:
		sys.argv.append('show')
	cmd = sys.argv[1].lower()
	if cmd == 'show':
		show()
	elif cmd == 'clean' or cmd == 'testclean':
		clean(cmd == 'testclean')
	elif argc == 3 and cmd == 'append':
		append(sys.argv[2])
	elif argc == 3 and cmd == 'remove':
		remove(sys.argv[2])
	else:
		print usage
		sys.exit()
