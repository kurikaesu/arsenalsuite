#!/usr/bin/python

import os,sys
sys.path.insert(0,os.path.join(os.getcwd(),"python/"))
path = os.path.dirname(os.path.abspath(__file__))

import cpp
import python
from blur.build import *
import blur.build

# Setup config file replacements, could read from multiple files or a more
# dynamic system if needed(database)
blur.build.Config_Replacement_File = os.path.join(path,'config_replacements.ini')

All_Targets.append( Target('allrpms',os.path.abspath(os.getcwd()),[
		"stonerpm","classesrpm","freezerrpm","absubmitrpm", "burnerrpm", "abscriptsrpm"]) )

if __name__ == "__main__":
	build()

