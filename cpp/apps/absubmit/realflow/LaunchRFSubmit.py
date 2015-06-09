import os
import sys
nT=int(os.environ['NUMBER_OF_PROCESSORS']) 


name = scene.getFileName()
root = scene.getRootPath()
threads = nT
filePath = str(root) + "\\" + str(name)
mStartFrame = 1
mEndFrame = scene.getMaxFrames() 


cmdString = "python " + "c:\\blur\\absubmit\\realflow\\submit.py " +  "fileName " + filePath + " mStartFrame " + str(mStartFrame) + " mEndFrame " + str(mEndFrame) + " mThreads " + str(threads)
cmdString += " version 4"

os.system(cmdString)