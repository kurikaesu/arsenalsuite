def printf(string, args=None, newline=True):
	if args == None:
		args = ()
	if newline:
		print(string % args)
	else:
		print(string % args, end=" ")