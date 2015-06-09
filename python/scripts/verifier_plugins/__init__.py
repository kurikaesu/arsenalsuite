import traceback
# copy all 4 lines for each plugin
#try:
#    import denyjob
#    reload(denyjob)
#except: traceback.print_exc()

#Temporarily re-enabled for week 05/10/11 - Finalling
#try:
#    import adjustPriority
#    reload(adjustPriority)
#except: traceback.print_exc()

#try:
#    import addServices
#    reload(addServices)
#except: traceback.print_exc()

#try:
#    import addFastLane
#    reload(addFastLane)
#except: traceback.print_exc()

try:
    import maxTaskTime
    reload(maxTaskTime)
except: traceback.print_exc()

try:
    import setShotName
    reload(setShotName)
except: traceback.print_exc()

#try:
#    import urgentPrios
#    reload(urgentPrios)
#except: traceback.print_exc()

try:
    import trailerPrios
    reload(trailerPrios)
except: traceback.print_exc()

try:
    import checkOnTens
    reload(checkOnTens)
except: traceback.print_exc()

try:
    import historicalSettings
    reload(historicalSettings)
except: traceback.print_exc()

