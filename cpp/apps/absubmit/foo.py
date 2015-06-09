import py2ab

s = {}
# required
s["jobType"] = "3Delight"
s["job"] = "arturBake"
s["packetSize"] = "1"
s["frameList"] = "1-1"

job = py2ab.farmSubmit(s)
#py2ab.stop()

