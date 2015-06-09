
import _winreg
import os

def set_registry_value(reg, key, value_name, value, value_type=_winreg.REG_SZ):
	try:
		k = _winreg.OpenKey(reg, key, 0, _winreg.KEY_WRITE)
	except:
		self.key = wreg.CreateKey(reg, key)
	_winreg.SetValueEx(k, value_name, 0, value_type, value)
	k.Close()


def get_registry_value(reg, key, value_name):
	k = _winreg.OpenKey(reg,key, 0, _winreg.KEY_READ)
	val = _winreg.QueryValueEx(k, value_name)
	k.Close()
	return val

reg = _winreg.ConnectRegistry(None, _winreg.HKEY_LOCAL_MACHINE)
key = r"SYSTEM\CurrentControlSet\Control\Session Manager\Environment"

(path,type) = get_registry_value(reg, key, "Path")

print "Orig:", path
components = path.split(';')
normalized = []
results = []

for c in components:
	c = c.replace( '/', '\\' ).replace( '\\\\', '\\' )
	n = c.lower()
	if not n in normalized:
		if not os.path.exists( c ) and not '%' in c:
			print "Path doesnt exist:", c
		else:
			results.append( c )
			normalized.append( n )
	else:
		print 'Skipping duplicate path:', c

print "Result:", ';'.join(results)
set_registry_value(reg, key, "Path", ';'.join(results), _winreg.REG_EXPAND_SZ)
