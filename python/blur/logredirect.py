
try:
	from blur.Stone import Log
	import sys
	
	class LogRedirect(object):
		def __init__(self,passThrough = None):
			self.queue = ''
			self.passThrough = passThrough
		def write(self,data):
			try:
				self.passThrough.write(data)
			except: pass
			self.queue += data
			if len(self.queue) and self.queue[-1] == '\n':
				self.flush()
		def flush(self):
			Log(self.queue.rstrip('\n'))
			self.queue = ''
	
	def RedirectOutputToLog( passThrough = False ):
		if not sys.stdout.__class__ == LogRedirect:
			if passThrough:
				sys.stderr = LogRedirect(sys.stderr)
				sys.stdout = LogRedirect(sys.stdout)
			else:
				sys.stderr = LogRedirect()
				sys.stdout = LogRedirect()
	
#	RedirectOutputToLog()
	
except: pass
