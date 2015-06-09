#!/usr/bin/python
from copy import copy

class DefaultDict(dict):
	def __init__( self, defaultValueClass = int, passKey = False ):
		self.defaultValueClass = defaultValueClass
		self.passKey = passKey
	def __getitem__( self, key ):
		try:
			return dict.__getitem__(self,key)
		except:
			try:
				if self.passKey:
					self[key] = self.defaultValueClass(key)
				else:
					self[key] = self.defaultValueClass()
			except:
				cp = copy(self.defaultValueClass)
				print "Setting %s to %s" % (hash(key),str(cp))
				self[key] =  cp
		return dict.__getitem__(self,key)


if __name__ == "__main__":
	test1 = DefaultDict(2)
	test1['a'] += 1
	print test1['a']

	test2 = DefaultDict( 1.5 )
	print test2[1]

	test3 = DefaultDict( DefaultDict( 'hello world' ) )
	print test3['a']['b']

	test4 = DefaultDict( int, True )
	print test4[1]

