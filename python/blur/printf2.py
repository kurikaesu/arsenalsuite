def printf(string, args = None, newline=True):
    if newline:
        if args == None:
            print string
        else:
            print string % args
    else:
        if args == None:
            print string,
        else:
            print string % args, 