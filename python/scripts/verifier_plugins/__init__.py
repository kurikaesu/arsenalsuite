import traceback

try:
    import maxTaskTime
    reload(maxTaskTime)
except: traceback.print_exc()

try:
    import setShotName
    reload(setShotName)
except: traceback.print_exc()

try:
    import checkOnTens
    reload(checkOnTens)
except: traceback.print_exc()
