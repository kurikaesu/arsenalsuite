import py2ab
import subprocess
import sys

jobFile = sys.argv[1]
lines = sys.argv[2]

s = {}
# required
s["jobType"] = "Batch"
s["user"] = "barry.robison"
s["job"] = jobFile
s["fileName"] = jobFile
s["packetSize"] = "100"
s["frameStart"] = "1"
s["frameEnd"] = str(lines)
s["cmd"] = "/drd/software/int/farm/multibash.sh %s" % jobFile
s["passSlaveFramesAsParam"] = "true"

# by default your current environment will be used. If you need a different apps env you can do this:
env = subprocess.Popen(["/drd/software/int/bin/launcher.sh","-p", "hf2", "-d", "rnd", "-e", "bach"], stdout=subprocess.PIPE).communicate()[0]
s["environment"] = env

job = py2ab.farmSubmit(s)

s = {}
s["jobType"] = "Batch"
s["user"] = "barry.robison"
s["job"] = "%s_cleanup" % jobFile
s["fileName"] = jobFile
s["packetSize"] = "1"
s["frameStart"] = "1"
s["frameEnd"] = "1"
s["cmd"] = "rm -f %s" % jobFile
s["passSlaveFramesAsParam"] = "false"
s["deps"] = str(job.key())
py2ab.farmSubmit(s)

sys.exit(0)

