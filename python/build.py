#!/usr/bin/python

import os, sys

path = os.path.dirname(os.path.abspath(__file__))

if __name__ == "__main__":
	sys.path.insert(0,path)

# Instead of 
# from blur.build import *
# we use this, because the former will re-import the module
# based on the local path, causing All_Targets to be
# a different list
locals().update( sys.modules['blur.build'].__dict__ )

if sys.platform=="linux2":
	#All_Targets.append( RPMTarget("blurpythonrpm",'blur-python',os.path.join(path,'blur'),'../../rpm/spec/blur-python.spec.template','1.0') )

	scripts_path = os.path.join(path,'scripts')
	'''
	All_Targets.append( RPMTarget("energysaverrpm",'energy_saver',scripts_path,'../../rpm/spec/energy_saver.spec.template','1.0') )
	All_Targets.append( RPMTarget("managerrpm",'manager',scripts_path,'../../rpm/spec/manager.spec.template','1.0') )
	All_Targets.append( RPMTarget("notifierrpm",'notifier',scripts_path,'../../rpm/spec/notifier.spec.template','1.0') )
	All_Targets.append( RPMTarget("reaperrpm",'reaper',scripts_path,'../../rpm/spec/reaper.spec.template','1.0') )
	All_Targets.append( RPMTarget("reclaimtasksrpm",'reclaim_tasks',scripts_path,'../../rpm/spec/reclaim_tasks.spec.template','1.0') )
	All_Targets.append( RPMTarget("joberrorhandlerrpm",'joberror_handler',scripts_path,'../../rpm/spec/joberror_handler.spec.template','1.0') )
	All_Targets.append( RPMTarget("rrdstatscollectorrpm", 'rrd_stats_collector',scripts_path,'../../rpm/spec/rrd_stats_collector.spec.template','1.0') )
	All_Targets.append( RPMTarget("unassigntasksrpm", 'unassign_tasks', scripts_path, '../../rpm/spec/unassign_tasks.spec.template','1.0') )
	All_Targets.append( RPMTarget("renderhostcheckerrpm", 'render_host_checker', scripts_path, '../../rpm/spec/render_host_checker.spec.template','1.0') )
	'''

	All_Targets.append( RPMTarget("abscriptsrpm","abscripts",scripts_path, "../../rpm/spec/abscripts.spec.template", "1.0", ['abscripts'] ) )
	
if __name__ == "__main__":
	build()

